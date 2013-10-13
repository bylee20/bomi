#include "audiofilter.hpp"
#include "audiocontroller.hpp"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifdef clamp
#undef clamp
#endif

bool AudioFilter::reconfigure(const mp_audio *data) {
	if (m_format != data->format)
		return false;
	m_channels = data->channels.num;
	m_samplerate = data->rate;
	m_bps = data->bps;
	reinitialize();
	return true;
}

bool AudioFilter::prepare(const AudioController *ac, const mp_audio *data) {
	if (!isCompatibleWith(data) && !reconfigure(data))
		return false;
	prepareToPlay(ac, data);
	return true;
}

template<typename T, typename B = T> using IfInteger = typename std::enable_if< Tmp::isInteger<T>(), B>::type;
template<typename T, typename B = T> using IfFloat   = typename std::enable_if<!Tmp::isInteger<T>(), B>::type;

template<typename T> constexpr IfInteger<T> maximum() { return std::numeric_limits<T>::max(); }
template<typename T> constexpr IfFloat<T>   maximum() { return (T)1.0; }

template<typename T> constexpr double toLevel(IfInteger<T> p) { return (double)qAbs(p)/(double)std::numeric_limits<T>::max(); }
template<typename T> constexpr double toLevel(IfFloat<T> p)   { return (double)qAbs(p); }

template<typename T> constexpr T            hardclip(double p) { return qBound(-(double)maximum<T>(), p, (double)maximum<T>()); }
template<typename T> constexpr IfFloat<T>   softclip(double p) { return (p >= M_PI*0.5) ? 1.0 : ((p <= -M_PI*0.5) ? -1.0 : sin(p)); }
template<typename T> constexpr IfInteger<T> softclip(double p) { return maximum<T>()*softclip<double>(p/maximum<T>()); }
template<typename T> constexpr IfInteger<T> autoclip(double p) { return hardclip<T>(p); }
template<typename T> constexpr IfFloat<T>   autoclip(double p) { return softclip<T>(p); }

template<ClippingMethod method, typename T> struct Clip { };
template<typename T> struct Clip<ClippingMethod::Auto, T> { static T apply(double p) { return autoclip<T>(p); } };
template<typename T> struct Clip<ClippingMethod::Hard, T> { static T apply(double p) { return hardclip<T>(p); } };
template<typename T> struct Clip<ClippingMethod::Soft, T> { static T apply(double p) { return softclip<T>(p); } };

template<typename T, const int s, const bool is_int = Tmp::isInteger<T>()>
struct ShiftInt { static void apply(T &t) { t >>= s; } };
template<typename T, const int s>
struct ShiftInt<T, s, false> { static void apply(T &t) { Q_UNUSED(t); } };

template<typename T>
class TempoScalerImpl : public TempoScaler {
public:
	TempoScalerImpl(int format): TempoScaler(format) {}

	static constexpr const bool isInt = Tmp::isInteger<T>();
	static constexpr const int bps = sizeof(T);
	static constexpr const int bits = bps*8;

	using SampleType = T;
	using MaxType = typename Tmp::Ternary<isInt, qint64, double>::ans;
	using CorrType = MaxType;
	using TableType = MaxType;

	void reset(const AudioController *ac) {
		m_scale = ac->scale();
		m_activated = ac->isTempoScalerActivated();
		if (needToApply()) {
			m_frames_stride_error  = 0;
			m_samples_stride_scaled  = m_scale * m_samples_stride;
			m_frames_stride_scaled = m_scale * s2f(m_samples_stride);
			m_mul = (double)m_samples_stride / m_samples_stride_scaled;
			m_samples_to_slide = m_samples_queued = 0;
			m_overlap.fill(0);
		}
	}

	void prepareToPlay(const AudioController *ac, const mp_audio *data) {
		m_delay = 0;
		if (m_scale != ac->scale() || m_activated != ac->isTempoScalerActivated())
			reset(ac);
		if (!needToApply())
			return;
		const int max_samples_out = ((int)(b2s(data->len) / m_samples_stride_scaled) + 1) * m_samples_stride;
		if (max_samples_out > m_buffer.size())
			m_buffer.resize(max_samples_out);

		int samples_offset_in = fill_queue(data, 0);
		m_buffer_end = m_buffer.data();
		while (m_samples_queued >= m_queue.size()) {
			int samples_off = 0;

			// output stride
			if (m_samples_overlap> 0) {
				if (m_frames_search > 0)
					samples_off = best_overlap_samples_offset();
				output_overlap(m_buffer_end, samples_off);
			}
			std::copy_n(m_queue.data() + samples_off + m_samples_overlap, m_samples_standing, m_buffer_end + m_samples_overlap);
			m_buffer_end += m_samples_stride;

			// input stride
			std::copy_n(m_queue.data() + samples_off + m_samples_stride, m_samples_overlap, m_overlap.data());
			const auto target_frames = m_frames_stride_scaled + m_frames_stride_error;
			const auto target_frames_integer = (int)target_frames;
			m_frames_stride_error = target_frames - target_frames_integer;
			m_samples_to_slide = f2s(target_frames_integer);

			samples_offset_in += fill_queue(data, samples_offset_in);
		}
		m_delay = s2b(m_samples_queued - m_samples_to_slide);
	}


	mp_audio* play(mp_audio* data) override {
		if (needToApply()) {
			Q_ASSERT(m_buffer_end != nullptr);
			data->audio = m_buffer.data();
			data->len   = s2b(m_buffer_end - m_buffer.data());
		}
		return data;
	}
	void reinitialize() override {
		const double frames_per_ms = fps() / 1000.0;
		const int samples_per_frames = channels();
		const int frames_stride = frames_per_ms * m_ms_stride;
		const int frames_overlap = qMax(0, int(frames_stride * m_percent_overlap));

		m_frames_search = (frames_overlap > 1) ? frames_per_ms * m_ms_search : 0;
		m_samples_stride = f2s(frames_stride);
		m_samples_overlap = f2s(frames_overlap);
		m_samples_standing = m_samples_stride - m_samples_overlap;
		m_overlap.resize(m_samples_overlap);
		if (m_overlap.isEmpty())
			return;

		m_table_blend.resize(m_samples_overlap);
		m_buf_pre_corr.resize(m_samples_overlap);
		m_table_window.resize(m_samples_overlap-samples_per_frames);
		m_queue.resize(f2s(m_frames_search) + m_samples_overlap + m_samples_stride);

		auto pb = m_table_blend.data();
		MaxType blend = isInt ? 65536 : 1.0;
		for (int i=0; i<frames_overlap; ++i) {
			const TableType v = blend*i / frames_overlap;
			pb = std::fill_n(pb, samples_per_frames, v);
		}

		const MaxType t = frames_overlap;
		const MaxType n = isInt ? 8589934588LL / (t * t) : 1.0;  // 4 * (2^31 - 1) / t^2
		auto pw = m_table_window.data();
		for (int i=1; i<frames_overlap; ++i) {
			TableType v = ( i * (t - i) * n );
			ShiftInt<TableType, 15>::apply(v);
			pw = std::fill_n(pw, samples_per_frames, v);
		}
	}
private:
	int fill_queue(const mp_audio* data, int samples_offset) {
		int samples_in = b2s(data->len) - samples_offset;
		int offset_unchanged = samples_offset;

		if (m_samples_to_slide > 0) {
			if (m_samples_to_slide < m_samples_queued) {
				int samples_move = m_samples_queued - m_samples_to_slide;
				std::copy_n(m_queue.constData() + m_samples_to_slide, samples_move, m_queue.data());
				m_samples_to_slide = 0;
				m_samples_queued = samples_move;
			} else {
				int samples_skip;
				m_samples_to_slide -= m_samples_queued;
				samples_skip = MPMIN(m_samples_to_slide, samples_in);
				m_samples_queued = 0;
				m_samples_to_slide -= samples_skip;
				samples_offset += samples_skip;
				samples_in -= samples_skip;
			}
		}

		if (samples_in > 0) {
			int samples_copy = MPMIN(m_queue.size() - m_samples_queued, samples_in);
			Q_ASSERT(samples_copy >= 0);
			std::copy_n((const SampleType*)data->audio + samples_offset, samples_copy, m_queue.data() + m_samples_queued);
			m_samples_queued += samples_copy;
			samples_offset += samples_copy;
		}

		return samples_offset - offset_unchanged;
	}

	int best_overlap_samples_offset() {
		const int samples_per_frame = channels();
		auto pw  = m_table_window.constData();
		auto po  = m_overlap.constData() + samples_per_frame;
		auto ppc = m_buf_pre_corr.data();
		if (isInt) {
			for (int i=samples_per_frame; i<m_samples_overlap; ++i)
				*ppc++ = qint64( *pw++ * *po++ ) >> 15;
		} else {
			for (int i=samples_per_frame; i<m_samples_overlap; ++i)
				*ppc++ = *pw++ * *po++;
		}

		MaxType best_corr = INT64_MIN;
		int best_off = 0;
		auto search_start = m_queue.constData() + samples_per_frame;
		for (int off=0; off<m_frames_search; ++off) {
			MaxType corr = 0;
			auto ppc = m_buf_pre_corr.constData();
			auto ps  = search_start;
			for (int i=samples_per_frame; i<m_samples_overlap; ++i)
				corr += *ppc++ * *ps++;
			if (corr > best_corr) {
				best_corr = corr;
				best_off  = off;
			}
			search_start += samples_per_frame;
		}
		return f2s(best_off);
	}

	void output_overlap(SampleType *pout, int samples_off) const {
		auto pb = m_table_blend.constData();
		auto po = m_overlap.constData();
		auto pin = m_queue.data() + samples_off;
		if (isInt) {
			for (int i=0; i<m_samples_overlap; ++i, ++po)
				*pout++ = *po - ( qint64( *pb++ * ( *po - *pin++ ) ) >> 16 );
		} else {
			for (int i=0; i<m_samples_overlap; ++i, ++po)
				*pout++ = *po - *pb++ * ( *po - *pin++ );
		}
	}

	bool needToApply() const { return m_activated && m_scale != 1.0; }

	static constexpr const double m_ms_stride = 60.0;
	static constexpr const double m_percent_overlap = 0.20;
	static constexpr const double m_ms_search = 14.0;

	bool m_activated = false;

	double m_scale = 1.0;
	double m_frames_stride_scaled = 0.0;
	double m_frames_stride_error = 0.0;
	double m_samples_stride_scaled = 0.0;

	int m_frames_search = 0;
	int m_samples_overlap = 0;
	int m_samples_standing = 0;
	int m_samples_stride = 0;
	int m_samples_queued = 0;
	int m_samples_to_slide = 0;

	QVector<SampleType> m_queue, m_overlap, m_buffer;
	QVector<TableType> m_table_blend, m_table_window;
	QVector<CorrType> m_buf_pre_corr;
	SampleType *m_buffer_end = nullptr;
};

template<ClippingMethod method, typename T>
class VolumeControllerImpl : public VolumeController {
	struct BufferInfo {
		BufferInfo(int frames = 0): frames(frames) {}
		int frames = 0; double level = 0.0;
	};
public:
	VolumeControllerImpl(int format): VolumeController(format, method) {
		qDebug() << "Clipping Method:" << ClippingMethodInfo::name(method);
	}
	mp_audio *play(mp_audio *data) override {
		auto p = (T*)(data->audio);
		if (m_muted)
			std::fill_n(p, b2s(data->len), 0);
		else {
			const int nch = channels();
			const int frames = s2f(b2s(data->len));
			for (int i=0; i<frames; ++i) {
				for (int ch = 0; ch < nch; ++ch, ++p)
					*p = Clip<method, T>::apply((*p)*m_level[ch]*m_gain);
			}
		}
		return data;
	}
private:
	void reinitialize() {
		m_gain = 1.0;
		m_buffers.clear();
	}
	BufferInfo getInfo(const mp_audio *data) const {
		const int samples = b2s(data->len);
		BufferInfo info(s2f(samples));
		auto p = static_cast<const T*>(data->audio);
		for (int i=0; i<samples; ++i)
			info.level += toLevel<T>(*p++);
		info.level /= samples;
		return info;
	}
	BufferInfo calculateAverage(const BufferInfo &data) const {
		BufferInfo total;
		for (const auto &one : m_buffers) {
			total.level += one.level*one.frames;
			total.frames += one.frames;
		}
		total.level += data.level*data.frames;
		total.frames += data.frames;
		total.level /= total.frames;
		return total;
	}
	void prepareToPlay(const AudioController *ac, const mp_audio *data) override {
		m_muted = ac->isMuted();
		const int nch = channels();
		std::fill_n(m_level, nch, ac->level());
		if (_Change(m_normalizer, ac->isNormalizerActivated()))
			m_buffers.clear();
		if (_Change(m_option, ac->normalizerOption()))
			m_buffers.clear();
		if (!m_normalizer || data->len <= 0) {
			m_gain = 1.0;
			return;
		}
		const auto info = getInfo(data);
		const auto avg = calculateAverage(info);
		const double targetGain = m_option.gain(avg.level);
		if (targetGain < 0)
			m_gain = 1.0;
		else {
			const double rate = targetGain/m_gain;
			if (rate > 1.05) {
				m_gain *= 1.05;
			} else if (rate < 0.95)
				m_gain *= 0.95;
			else
				m_gain = targetGain;
		}
		if ((double)avg.frames/(double)fps() >= m_option.bufferLengthInSeconds) {
			if (++m_it == m_buffers.end())
				m_it = m_buffers.begin();
			*m_it = info;
		} else {
			m_buffers.push_back(info);
			m_it = --m_buffers.end();
		}
	}

	bool m_normalizer = false, m_muted = false;
	double m_level[AF_NCH];
	NormalizerOption m_option;
	QLinkedList<BufferInfo> m_buffers;
	typename QLinkedList<BufferInfo>::iterator m_it;
};

#define DEC_CREATE(Class) \
Class *Class::create(int format) { \
	switch (format) { \
	case AF_FORMAT_S16_NE:    return new Class##Impl<qint16>(format); \
	case AF_FORMAT_S32_NE:    return new Class##Impl<qint32>(format); \
	case AF_FORMAT_FLOAT_NE:  return new Class##Impl<float> (format);  \
	case AF_FORMAT_DOUBLE_NE: return new Class##Impl<double>(format); \
	default:			   return nullptr; \
	} \
}
DEC_CREATE(TempoScaler)

template<ClippingMethod method>
VolumeController *create(int format) {
	switch (format) {
	case AF_FORMAT_S16_NE:
		return new VolumeControllerImpl<method, qint16>(format);
	case AF_FORMAT_S32_NE:
		return new VolumeControllerImpl<method, qint32>(format);
	case AF_FORMAT_FLOAT_NE:
		return new VolumeControllerImpl<method, float> (format);
	case AF_FORMAT_DOUBLE_NE:
		return new VolumeControllerImpl<method, double>(format);
	default:
		return nullptr;
	}
}

VolumeController *VolumeController::create(int format, ClippingMethod method) {
	switch (method) {
	case ClippingMethod::Auto:
		return ::create<ClippingMethod::Auto>(format);
	case ClippingMethod::Soft:
		return ::create<ClippingMethod::Soft>(format);
	case ClippingMethod::Hard:
		return ::create<ClippingMethod::Hard>(format);
	default:
		return nullptr;
	}
}
