#include "audiomixer.hpp"
#include "audio_helper.hpp"

template<int fmt_in>
class AudioScaler {
public:
	using Trait = AudioFormatTrait<fmt_in>;
	using S = typename Trait::SampleType;
	using Helper = AudioSampleHelper<S>;
	using MaxType = typename std::conditional<Trait::IsInt, qint64, double>::type;
	using CorrType = MaxType;
	using TableType = MaxType;
	AudioScaler() {}
	static constexpr int fmt_out = to_interleaving<fmt_in>::value;

	double delay() const { return m_delay; }

	void setFormat(const AudioDataFormat &format) {
		m_format = format;
		const int nch = m_format.channels.num;
		const double frames_per_ms = m_format.fps / 1000.0;
		m_frames_stride = frames_per_ms * m_ms_stride;
		m_frames_overlap = qMax<int>(0, m_frames_stride * m_percent_overlap);

		m_frames_search = (m_frames_overlap > 1) ? frames_per_ms * m_ms_search : 0;
		m_frames_standing = m_frames_stride - m_frames_overlap;
		m_overlap.setData(m_frames_overlap, nch);
		if (m_overlap.isEmpty())
			return;

		m_table_blend.setData(m_frames_overlap, nch);
		const MaxType blend = Trait::IsInt ? 65536 : 1.0;
		for (int i=0; i<m_frames_overlap; ++i)
			m_table_blend.fill(i, blend*i/m_frames_overlap);

		m_table_window.setData(m_frames_overlap-1, nch);
		const MaxType t = m_frames_overlap;
		const MaxType n = Trait::IsInt ? 8589934588LL / (t * t) : 1.0;  // 4 * (2^31 - 1) / t^2
		for (int i=1; i<m_frames_overlap; ++i)
			m_table_window.fill(i-1, rshift<15, TableType>(i*(t - i)*n));

		m_buf_pre_corr.setData(m_frames_overlap, nch);
		m_queue.setData(m_frames_search + m_frames_overlap + m_frames_stride, nch);

		m_src = {nch}; m_dst = {nch};
	}

	void setScale(bool on, double scale) {
		m_scale = scale;
		m_frames_stride_error = 0;
		m_frames_stride_scaled = m_scale * m_frames_stride;
		m_frames_to_slide = m_frames_queued = 0;
		m_overlap.fill(0);
		m_enabled = on && scale != 1.0;
	}

	bool adjusted(const mp_audio *in) {
		m_src.setData(in);
		m_delay = 0;
		if (!m_enabled || in->samples <= 0)
			return false;
		const int max_frames_out = ((int)(in->samples / m_frames_stride_scaled) + 1) * m_frames_stride;
		if (!m_dst.expand(max_frames_out))
			m_dst.adjust(max_frames_out);
		int frames_offset_in = fill_queue(0);
		int frames_out = 0;
		while (m_frames_queued >= m_queue.frames()) {
			int frames_off = 0;

			// output stride
			if (m_frames_overlap> 0) {
				if (m_frames_search > 0)
					frames_off = best_overlap_frames_offset();
				output_overlap(frames_out, frames_off);
			}

			m_dst.copy(frames_out + m_frames_overlap, m_queue, frames_off + m_frames_overlap, m_frames_standing);
			frames_out += m_frames_stride;

			// input stride
			m_overlap.copy(0, m_queue, frames_off + m_frames_stride, m_frames_overlap);
			const auto target_frames = m_frames_stride_scaled + m_frames_stride_error;
			const auto target_frames_integer = (int)target_frames;
			m_frames_stride_error = target_frames - target_frames_integer;
			m_frames_to_slide = target_frames_integer;
			frames_offset_in += fill_queue(frames_offset_in);
		}
		m_delay = (m_frames_queued - m_frames_to_slide)/m_scale/m_format.fps;
		m_dst.adjust(frames_out);
		return true;
	}
	const AudioDataBuffer<S, false> &output() const { return m_dst; }
private:
	template<int s, typename T>
	constexpr inline T rshift(const T &t) const { return Helper::template rshift<s, T>(t); }

	int fill_queue(int frames_offset) {
		int frames_in = m_src.frames() - frames_offset;
		const int offset_unchanged = frames_offset;

		if (m_frames_to_slide > 0) {
			if (m_frames_to_slide < m_frames_queued) {
				const int frames_move = m_frames_queued - m_frames_to_slide;
				m_queue.move(0, m_queue, m_frames_to_slide, frames_move);
				m_frames_to_slide = 0;
				m_frames_queued = frames_move;
			} else {
				m_frames_to_slide -= m_frames_queued;
				const int frames_skip = qMin(m_frames_to_slide, frames_in);
				m_frames_queued = 0;
				m_frames_to_slide -= frames_skip;
				frames_offset += frames_skip;
				frames_in -= frames_skip;
			}
		}

		if (frames_in > 0) {
			const int frames_copy = qMin(m_queue.frames() - m_frames_queued, frames_in);
			m_queue.copy(m_frames_queued, m_src, frames_offset, frames_copy);
			m_frames_queued += frames_copy;
			frames_offset += frames_copy;
		}
		return frames_offset - offset_unchanged;
	}

	void calculate_correlations() {
		_AudioManipulate([&] (CorrType &c, TableType w, S o) { c = rshift<15, MaxType>(w*o); }
			, m_buf_pre_corr, 0, m_overlap.frames()-1, m_table_window, 0, m_overlap, 1);
	}

	int best_overlap_frames_offset() {
		calculate_correlations();
		int best_off = 0; MaxType best_corr = _Min<qint64>(), corr;
		for (int off=0; off<m_frames_search; ++off) {
			corr = 0;
			_AudioManipulate([&] (CorrType c, S q) { corr += c*q; }
				, _C(m_buf_pre_corr), 0, m_overlap.frames()-1, _C(m_queue), 1+off);
			if (corr > best_corr) {
				best_corr = corr;
				best_off  = off;
			}
		}
		return best_off;
	}

	void output_overlap(int pos, int frames_off) {
		_AudioManipulate([&] (S &d, TableType b, S o, S q) {
			d = o - rshift<16, MaxType>(b*(o - q));
		} , m_dst, pos, m_overlap.frames(), m_table_blend, 0, m_overlap, 0, m_queue, frames_off);
	}
	static constexpr const double m_ms_stride = 60.0;
	static constexpr const double m_percent_overlap = 0.20;
	static constexpr const double m_ms_search = 14.0;

	AudioDataFormat m_format;
	bool m_enabled = false;
	double m_frames_stride_scaled = 0.0, m_frames_stride_error = 0.0;
	int m_frames_stride = 0, m_frames_overlap = 0, m_frames_queued = 0;
	int m_frames_search = 0, m_frames_standing = 0, m_frames_to_slide = 0;

	AudioDataBuffer<TableType, false> m_table_blend, m_table_window;
	AudioDataBuffer<CorrType, false> m_buf_pre_corr;
	AudioDataBuffer<S, false> m_queue, m_overlap;
	double m_delay = 0.0, m_scale = 1.0;

	AudioDataBuffer<S, Trait::IsPlanar> m_src;
	AudioDataBuffer<S, false> m_dst;
};

template<int fmt_src, int fmt_dst, ClippingMethod method>
class AudioMixerImpl : public AudioMixer {
public:
	static_assert(method != ClippingMethod::Auto, "invalid specialization");
	using Trait = AudioFormatTrait<fmt_src>;
	template<class T>
	using Helper = AudioSampleHelper<T>;
	using S = typename Trait::SampleType;
	using D = typename AudioFormatTrait<fmt_dst>::SampleType;
	static constexpr D trans(S s) { return Helper<S>::template conv<D>(Helper<S>::template clip<method>(s)); }
	AudioMixerImpl(const AudioDataFormat &in, const AudioDataFormat &out)
	: AudioMixer(in, out, method) { }
	void apply(const mp_audio *in) override {
		if (m_scaler.adjusted(in)) {
			process(m_scaler.output());
		} else
			process<Trait::IsPlanar>(in);
		m_delay = m_scaler.delay();
	}
	void setOutput(mp_audio *output) override { m_output = output; m_dst = {output}; }
	void configured() { m_scaler.setFormat(m_in); }
	void setScaler(bool on, double scale) override { m_scaler.setScale(on, scale); m_scale = on ? scale : 1.0; }
private:
	bool checkClippingMethod(ClippingMethod _method) const override {
		if (_method == ClippingMethod::Auto)
			return method == Trait::AutoClipping;
		return _method == method;
	}
	LevelInfo calculateAverage(const LevelInfo &add) const {
		LevelInfo total;
		for (const auto &one : m_history) {
			total.level += one.level*one.frames;
			total.frames += one.frames;
		}
		total.level += add.level*add.frames;
		total.frames += add.frames;
		total.level /= total.frames;
		return total;
	}

	template<bool planar>
	void process(const AudioDataBuffer<S, planar> &src) {
		prepare(src);
		if (m_dst.isEmpty())
			return;
		const auto gain = m_amp*m_gain;
		if (m_amp < 1e-8)
			m_dst.fill(0);
		else if (!m_mix) {
			_AudioManipulate([&] (D &out, S in) { out = trans(in*gain); }, m_dst, 0, m_dst.frames(), src, 0);
		} else {
			for (int frame = 0; frame<m_dst.frames(); ++frame) {
				const auto dchannels = m_dst.channels(frame);
				const auto schannels = _C(src).channels(frame);
				for (auto it = dchannels.begin(); it != dchannels.end(); ++it) {
					const int dch = it.channel();
					auto &map = m_ch_man.sources((mp_speaker_id)m_out.channels.speaker[dch]);
					double value = 0;
					for (int i=0; i<map.size(); ++i)
						value += schannels[m_ch_index_src[map[i]]]*gain;
					*it = trans(value);
				}
			}
		}
	}
	template<bool planar>
	void prepare(const AudioDataBuffer<S, planar> &src) {
		const int frames = src.frames();
		mp_audio_realloc_min(m_output, frames);
		m_output->samples = frames;
		m_dst = m_output;
		if (frames <= 0 || !m_normalizer)
			return;
		LevelInfo input(frames);
		_AudioManipulate([&] (S s) { input.level += Helper<S>::toLevel(s); }, src, 0, src.frames());
		input.level /= src.samples();
		const auto avg = calculateAverage(input);
		const double targetGain = m_normalizerOption.gain(avg.level);
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
		if (avg.frames/(double)m_in.fps >= m_normalizerOption.bufferLengthInSeconds) {
			if (++m_historyIt == m_history.end())
				m_historyIt = m_history.begin();
			*m_historyIt = input;
		} else {
			m_history.push_back(input);
			m_historyIt = --m_history.end();
		}
	}
	mutable AudioDataBuffer<D, AudioFormatTrait<fmt_dst>::IsPlanar> m_dst;
	AudioScaler<fmt_src> m_scaler;
};

template<int fmt_in, int fmt_out>
static AudioMixer *createImpl(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip) {
	switch (clip) {
	case ClippingMethod::Auto:
		return new AudioMixerImpl<fmt_in, fmt_out, AudioFormatTrait<fmt_in>::AutoClipping>(in, out);
	case ClippingMethod::Hard:
		return new AudioMixerImpl<fmt_in, fmt_out, ClippingMethod::Hard>(in, out);
	case ClippingMethod::Soft:
		return new AudioMixerImpl<fmt_in, fmt_out, ClippingMethod::Soft>(in, out);
	default:
		return nullptr;
	}
}

template<int fmt_in>
static AudioMixer *create1(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip) {
	switch (out.type) {
	case AF_FORMAT_S16:
		return createImpl<fmt_in, AF_FORMAT_S16>(in, out, clip);
	case AF_FORMAT_S32:
		return createImpl<fmt_in, AF_FORMAT_S32>(in, out, clip);
	case AF_FORMAT_FLOAT:
		return createImpl<fmt_in, AF_FORMAT_FLOAT>(in, out, clip);
	case AF_FORMAT_DOUBLE:
		return createImpl<fmt_in, AF_FORMAT_DOUBLE>(in, out, clip);
	case AF_FORMAT_S16P:
		return createImpl<fmt_in, AF_FORMAT_S16P>(in, out, clip);
	case AF_FORMAT_S32P:
		return createImpl<fmt_in, AF_FORMAT_S32P>(in, out, clip);
	case AF_FORMAT_FLOATP:
		return createImpl<fmt_in, AF_FORMAT_FLOATP>(in, out, clip);
	case AF_FORMAT_DOUBLEP:
		return createImpl<fmt_in, AF_FORMAT_DOUBLEP>(in, out, clip);
	default:
		return nullptr;
	}
}

AudioMixer *AudioMixer::create(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip) {
	auto make = [&in, &out, clip] () -> AudioMixer * {
		switch (in.type) {
		case AF_FORMAT_S16:
			return create1<AF_FORMAT_S16>(in, out, clip);
		case AF_FORMAT_S32:
			return create1<AF_FORMAT_S32>(in, out, clip);
		case AF_FORMAT_FLOAT:
			return create1<AF_FORMAT_FLOAT>(in, out, clip);
		case AF_FORMAT_DOUBLE:
			return create1<AF_FORMAT_DOUBLE>(in, out, clip);
		case AF_FORMAT_S16P:
			return create1<AF_FORMAT_S16P>(in, out, clip);
		case AF_FORMAT_S32P:
			return create1<AF_FORMAT_S32P>(in, out, clip);
		case AF_FORMAT_FLOATP:
			return create1<AF_FORMAT_FLOATP>(in, out, clip);
		case AF_FORMAT_DOUBLEP:
			return create1<AF_FORMAT_DOUBLEP>(in, out, clip);
		default:
			return nullptr;
		}
	};
	auto mixer = make();
	Q_ASSERT(mixer != nullptr);
	auto ok = mixer->configure(in, out, clip);
	Q_ASSERT(ok);
	return mixer;
}
