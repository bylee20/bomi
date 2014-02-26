#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include "enums.hpp"
#include "channelmanipulation.hpp"
extern "C" {
#include <audio/audio.h>
}

struct AudioDataFormat {
	AudioDataFormat() {}
	AudioDataFormat(const mp_audio &mpv)
	: fps(mpv.rate), type(mpv.format), channels(mpv.channels) {
	}
	bool operator == (const AudioDataFormat &rhs) const {
		return type == rhs.type && mp_chmap_equals(&channels, &rhs.channels) && fps == rhs.fps;
	}
	bool operator != (const AudioDataFormat &rhs) const { return !operator == (rhs); }
	int fps = 0, type = 0;
	mp_chmap channels;
};

struct AudioNormalizerOption {
	bool operator == (const AudioNormalizerOption &rhs) const {
		return silenceLevel == rhs.silenceLevel && targetLevel == rhs.targetLevel
			&& minimumGain == rhs.minimumGain && maximumGain == rhs.maximumGain
			&& bufferLengthInSeconds == rhs.bufferLengthInSeconds;
	}
	bool operator != (const AudioNormalizerOption &rhs) const { return !operator==(rhs); }
	double gain(double level) {
		return (level > silenceLevel) ? qBound(minimumGain, targetLevel / level, maximumGain) : -1.0;
	}
	double silenceLevel = 0.0001, minimumGain = 0.1, maximumGain = 10.0, targetLevel = 0.07, bufferLengthInSeconds = 5.0;
};

class AudioMixer {
public:
	static AudioMixer *create(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip);
	virtual ~AudioMixer() {}
	float gain() const { return m_gain; }
	bool configure(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip) {
		if (m_in.type != in.type || m_out.type != out.type || !checkClippingMethod(clip))
			return false;
		m_in = in; m_out = out;
		for (int i=0; i<out.channels.num; ++i)
			m_ch_index_dst[out.channels.speaker[i]] = i;
		for (int i=0; i<in.channels.num; ++i)
			m_ch_index_src[in.channels.speaker[i]] = i;
		m_ch_man = m_map(in.channels, out.channels);
		m_mix = !m_ch_man.isIdentity();
		m_updateChmap = !mp_chmap_equals(&in.channels, &out.channels);
		m_updateFormat = in.type != out.type;
		m_history.clear();
		m_historyIt = m_history.end();
		configured();
		return true;
	}
	double multiplier() const { return ((double)m_out.channels.num/m_in.channels.num)/m_scale; }
	double delay() const { return m_delay; }
	void setNormalizer(bool on, const AudioNormalizerOption &option) {
		m_normalizer = on;
		m_normalizerOption = option;
		m_gain = 1.0;
		m_history.clear();
		m_historyIt = m_history.end();
	}
	void setAmp(float level) { m_amp = level; }
	void setChannelLayoutMap(const ChannelLayoutMap &map) {
		m_map = map;
		m_ch_man = map(m_in.channels, m_out.channels);
	}
	virtual void apply(const mp_audio *in) = 0;
	virtual void setScaler(bool on, double scale) = 0;
	virtual void setOutput(mp_audio *output) = 0;
protected:
	virtual bool checkClippingMethod(ClippingMethod method) const = 0;
	virtual void configured() = 0;
	AudioMixer(const AudioDataFormat &in, const AudioDataFormat &out, ClippingMethod clip)
	: m_in(in), m_out(out), m_clip(clip) {  }

	AudioDataFormat m_in, m_out;
	struct LevelInfo { LevelInfo(int frames = 0): frames(frames) {} int frames = 0; double level = 0.0; };
	double m_delay = 0.0, m_scale = 1.0;
	float m_gain = 1.0, m_amp = 1.0;
	ClippingMethod m_clip;
	QLinkedList<LevelInfo> m_history;
	bool m_scaleChanged = false, m_mix = true;
	bool m_normalizer = false, m_updateChmap = false, m_updateFormat = false;
	typename QLinkedList<LevelInfo>::iterator m_historyIt;
	AudioNormalizerOption m_normalizerOption;
	std::array<int, MP_SPEAKER_ID_COUNT> m_ch_index_src, m_ch_index_dst;
	ChannelManipulation m_ch_man;
	ChannelLayoutMap m_map;
	mp_audio *m_output = nullptr;
};

#endif // AUDIOMIXER_HPP
