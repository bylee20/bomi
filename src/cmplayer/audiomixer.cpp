#include "audiomixer.hpp"
#include "audiomixer_p.hpp"

template<int fmt_in>
class AudioScaler {
public:
	using H = Help<fmt_in>;
	AudioScaler() {}
	using S = typename H::SampleType;
	using MaxType = typename std::conditional<H::IsInt, qint64, double>::type;
	using CorrType = MaxType;
	using TableType = MaxType;

	static constexpr int fmt_out = ToInterleaving<fmt_in>::value;

	double delay() const { return m_delay; }

	void setFormat(const AudioFormat &format) {
		m_format = format;
		const int nch = m_format.channels.num;
		const double frames_per_ms = m_format.fps / 1000.0;
		m_frames_stride = frames_per_ms * m_ms_stride;
		m_frames_overlap = qMax<int>(0, m_frames_stride * m_percent_overlap);

		m_frames_search = (m_frames_overlap > 1) ? frames_per_ms * m_ms_search : 0;
		m_frames_standing = m_frames_stride - m_frames_overlap;
		m_overlap.resize(m_frames_overlap, nch);
		if (m_overlap.isEmpty())
			return;

		m_table_blend.resize(m_frames_overlap, nch);
		const MaxType blend = H::IsInt ? 65536 : 1.0;
		for (int i=0; i<m_frames_overlap; ++i)
			m_table_blend.fill(i, blend*i/m_frames_overlap);

		m_table_window.resize(m_frames_overlap-1, nch);
		const MaxType t = m_frames_overlap;
		const MaxType n = H::IsInt ? 8589934588LL / (t * t) : 1.0;  // 4 * (2^31 - 1) / t^2
		for (int i=1; i<m_frames_overlap; ++i)
			m_table_window.fill(i-1, rshift<15, TableType>(i*(t - i)*n));

		m_buf_pre_corr.resize(m_frames_overlap, nch);
		m_queue.resize(m_frames_search + m_frames_overlap + m_frames_stride, nch);

		m_src = {nch}; m_dst = {nch};

		m_buffer = {nch};
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
		m_src.setBuffer(in);
		m_delay = 0;
		if (!m_enabled || in->samples <= 0)
			return false;
		const int max_frames_out = ((int)(in->samples / m_frames_stride_scaled) + 1) * m_frames_stride;
		if (_Expand(m_buffer, max_frames_out) || m_dst.isEmpty())
			m_dst = {m_format, max_frames_out, m_buffer.data()};
		m_dst.resize(max_frames_out);
		int frames_offset_in = fill_queue(0);
		int frames_out = 0;
		while (m_frames_queued >= m_queue.size()) {
			int frames_off = 0;

			// output stride
			if (m_frames_overlap> 0) {
				if (m_frames_search > 0)
					frames_off = best_overlap_frames_offset();
				output_overlap(frames_out, frames_off);
			}

			m_queue.copyTo(m_buffer.frame(frames_out + m_frames_overlap), frames_off + m_frames_overlap, m_frames_standing);
			frames_out += m_frames_stride;

			// input stride
			m_queue.copyTo(m_overlap.data(), frames_off + m_frames_stride, m_frames_overlap);
			const auto target_frames = m_frames_stride_scaled + m_frames_stride_error;
			const auto target_frames_integer = (int)target_frames;
			m_frames_stride_error = target_frames - target_frames_integer;
			m_frames_to_slide = target_frames_integer;
			frames_offset_in += fill_queue(frames_offset_in);
		}
		m_delay = (m_frames_queued - m_frames_to_slide)/m_scale/m_format.fps;
		m_dst.resize(frames_out);
		return true;
	}
	const AudioFrameIterator<fmt_out> &output() const { return m_dst; }
private:
	template<int s, typename T>
	constexpr inline T rshift(const T &t) const { return H::template rshift<s, T>(t); }

	int fill_queue(int frames_offset) {
		int frames_in = m_src.size() - frames_offset;
		const int offset_unchanged = frames_offset;

		if (m_frames_to_slide > 0) {
			if (m_frames_to_slide < m_frames_queued) {
				const int frames_move = m_frames_queued - m_frames_to_slide;
				m_queue.copyTo(m_queue.data(), m_frames_to_slide, frames_move);
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
			const int frames_copy = qMin(m_queue.size() - m_frames_queued, frames_in);
			m_src.seek(frames_offset);
			for (int i=0; i<frames_copy; ++i, m_src.next()) {
				Q_ASSERT(m_src.ok());
				auto frame = m_queue.frame(m_frames_queued+i);
				for (int ch=0; ch<m_queue.channels(); ++ch)
					frame[ch] = m_src.get(ch);
			}
			m_frames_queued += frames_copy;
			frames_offset += frames_copy;
		}
		return frames_offset - offset_unchanged;
	}

	void calculate_correlations() {
		const int nch = m_format.channels.num;
		auto pw  = _C(m_table_window).data();
		auto po  = _C(m_overlap).frame(1);
		auto ppc = m_buf_pre_corr.data();
		const int samples_overlap = m_overlap.samples();
		for (int i=nch; i<samples_overlap; ++i)
			*ppc++ = rshift<15, MaxType>(*pw++ * *po++);
	}

	int best_overlap_frames_offset() {
		calculate_correlations();

		const int samples_overlap = m_overlap.samples();
		const int nch = m_format.channels.num;
		int best_off = 0;
		MaxType best_corr = _Min<qint64>(), corr;
		auto search_start = _C(m_queue).frame(1);
		auto corr_start = _C(m_buf_pre_corr).data();
		const CorrType *pc; const S *ps;
		for (int off=0; off<m_frames_search; ++off) {
			corr = 0;
			pc = corr_start;
			ps = search_start;
			for (int i=nch; i<samples_overlap; ++i)
				corr += *pc++ * *ps++;
			if (corr > best_corr) {
				best_corr = corr;
				best_off  = off;
			}
			search_start += nch;
		}
		return best_off;
	}

	void output_overlap(int pos, int frames_off) {
		auto pb = _C(m_table_blend).data();
		auto po = _C(m_overlap).data();
		auto pin = m_queue.frame(frames_off);
		const int samples = m_overlap.samples();
		auto p = m_buffer.frame(pos);
		for (int i=0; i<samples; ++i, ++po)
			*p++ = *po - rshift<16, MaxType>(*pb++ * (*po - *pin++));
	}
	static constexpr const double m_ms_stride = 60.0;
	static constexpr const double m_percent_overlap = 0.20;
	static constexpr const double m_ms_search = 14.0;

	AudioFormat m_format;
	bool m_enabled = false;
	double m_frames_stride_scaled = 0.0, m_frames_stride_error = 0.0;
	int m_frames_stride = 0, m_frames_overlap = 0, m_frames_queued = 0;
	int m_frames_search = 0, m_frames_standing = 0, m_frames_to_slide = 0;

	AudioFrameBuffer<TableType> m_table_blend, m_table_window;
	AudioFrameBuffer<CorrType> m_buf_pre_corr;
	AudioFrameBuffer<S> m_queue, m_overlap;
	double m_delay = 0.0, m_scale = 1.0;

	AudioFrameIterator<fmt_in> m_src;
	AudioFrameIterator<fmt_out> m_dst;
	AudioFrameBuffer<S> m_buffer;
};

template<int fmt_src, int fmt_dst, ClippingMethod method>
class AudioMixerImpl : public AudioMixer {
public:
	using H = Help<fmt_src>;
	using T = typename H::SampleType;
	using D = typename Help<fmt_dst>::SampleType;
	static constexpr D trans(T t) { return H::template conv<D>(H::template clip<method>(t)); }
	AudioMixerImpl(const AudioFormat &in, const AudioFormat &out)
	: AudioMixer(in, out, method) { }
	void apply(const mp_audio *in) override {
		if (m_scaler.adjusted(in)) {
			process(m_scaler.output());
		} else
			process<fmt_src>(in);
		m_delay = m_scaler.delay();
	}
	void setOutput(mp_audio *output) override { m_output = output; m_dst = {output}; }
	void configured() { m_scaler.setFormat(m_in); }
	void setScaler(bool on, double scale) override { m_scaler.setScale(on, scale); m_scale = on ? scale : 1.0; }
private:
	LevelInfo calculateAverage(const LevelInfo &data, int ch) const {
		LevelInfo total;
		for (const auto &one : m_inputLevelHistory[ch]) {
			total.level += one.level*one.frames;
			total.frames += one.frames;
		}
		total.level += data.level*data.frames;
		total.frames += data.frames;
		total.level /= total.frames;
		return total;
	}

	template<int fmt>
	void process(const AudioFrameIterator<fmt> &src) {
		prepare(src);
		if (m_dst.size() <= 0)
			return;
		if (m_muted)
			m_dst.fill(0);
		else {
			for (src.start(), m_dst.start(); src.ok(); src.next(), m_dst.next()) {
				Q_ASSERT(m_dst.ok());
				for (int ch = 0; ch < m_dst.channels(); ++ch) {
					auto &sources = m_ch_man.sources((mp_speaker_id)m_out.channels.speaker[ch]);
					double value = 0;
					for (int s = 0; s<sources.size(); ++s) {
						const auto srcIdx = m_ch_index_src[sources[s]];
						value += src.get(srcIdx)*m_amp[srcIdx]*m_gain[srcIdx];
					}
					m_dst.get(ch) = trans(value);
				}
			}
		}
	}
	template<int fmt>
	void prepare(const AudioFrameIterator<fmt> &src) {
		const int frames = src.size();
		mp_audio_realloc_min(m_output, frames);
		m_output->samples = frames;
		m_dst = m_output;
		if (frames <= 0 || !m_normalizer)
			return;
		m_inputLevels.fill(LevelInfo(frames));
		src.for_ch([this, &src] (int ch) {
			m_inputLevels[ch].level += H::toLevel(src.get(ch));
		});
		for (auto &input : m_inputLevels)
			input.level /= frames;
		for (int ch = 0; ch < m_inputLevels.size(); ++ch) {
			auto &input = m_inputLevels[ch];
			auto &gain = m_gain[ch];
			const auto avg = calculateAverage(input, ch);
			const double targetGain = m_normalizerOption.gain(avg.level);
			if (targetGain < 0)
				gain = 1.0;
			else {
				const double rate = targetGain/gain;
				if (rate > 1.05) {
					gain *= 1.05;
				} else if (rate < 0.95)
					gain *= 0.95;
				else
					gain = targetGain;
			}
			auto &it = m_its[ch];
			auto &buffers = m_inputLevelHistory[ch];
			if ((double)avg.frames/(double)m_in.fps >= m_normalizerOption.bufferLengthInSeconds) {
				if (++it == buffers.end())
					it = buffers.begin();
				*it = input;
			} else {
				buffers.push_back(input);
				it = --buffers.end();
			}
		}
	}
	mutable AudioFrameIterator<fmt_dst> m_dst;
	AudioScaler<fmt_src> m_scaler;
};

template<int fmt_in, int fmt_out>
static AudioMixer *createImpl(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip) {
	switch (clip) {
	case ClippingMethod::Auto:
		return new AudioMixerImpl<fmt_in, fmt_out, ClippingMethod::Auto>(in, out);
	case ClippingMethod::Hard:
		return new AudioMixerImpl<fmt_in, fmt_out, ClippingMethod::Hard>(in, out);
	case ClippingMethod::Soft:
		return new AudioMixerImpl<fmt_in, fmt_out, ClippingMethod::Soft>(in, out);
	default:
		return nullptr;
	}
}

template<int fmt_in>
static AudioMixer *create1(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip) {
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

AudioMixer *AudioMixer::create(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip) {
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
