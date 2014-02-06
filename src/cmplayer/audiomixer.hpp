#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include "enums.hpp"
#include "channelmanipulation.hpp"
extern "C" {
#include <audio/audio.h>
}

struct AudioFormat {
	AudioFormat() {}
	AudioFormat(const mp_audio &mpv)
	: fps(mpv.rate), type(mpv.format), channels(mpv.channels) {
	}
	bool operator == (const AudioFormat &rhs) const {
		return type == rhs.type && mp_chmap_equals(&channels, &rhs.channels) && fps == rhs.fps;
	}
	bool operator != (const AudioFormat &rhs) const { return !operator == (rhs); }
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
	static AudioMixer *create(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip);
	virtual ~AudioMixer() {}
	float gain() const {
		float ret = 0.0; int count = 0;
		for (auto g : m_gain) { if (g > 1.0001f) { ret += g; ++count; } }
		return count > 0 ? ret/count : 1.0;
	}
	bool configure(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip) {
		if (m_in.type != in.type || m_out.type != out.type || m_clip != clip)
			return false;
		m_in = in; m_out = out;
		for (int i=0; i<out.channels.num; ++i)
			m_ch_index_dst[out.channels.speaker[i]] = i;
		for (int i=0; i<in.channels.num; ++i)
			m_ch_index_src[in.channels.speaker[i]] = i;
		m_ch_man = m_map(in.channels, out.channels);
		m_updateChmap = !mp_chmap_equals(&in.channels, &out.channels);
		m_updateFormat = in.type != out.type;
		m_inputLevels.resize(in.channels.num);
		m_its.resize(in.channels.num);
		m_inputLevelHistory.resize(in.channels.num);
		configured();
		return true;
	}
	double multiplier() const { return ((double)m_out.channels.num/m_in.channels.num)/m_scale; }
	double delay() const { return m_delay; }
	void setNormalizer(bool on, const AudioNormalizerOption &option) {
		m_normalizer = on;
		m_normalizerOption = option;
		std::fill_n(m_gain, MP_NUM_CHANNELS, 1.0);
		for (auto &it : m_inputLevelHistory)
			it.clear();
	}
	void setMuted(bool muted) { m_muted = muted; }
	void setAmp(float level) { std::fill_n(m_amp, MP_NUM_CHANNELS, level); }
	ClippingMethod clippingMethod() const { return m_clip; }
	void setChannelLayoutMap(const ChannelLayoutMap &map) {
		m_map = map;
		m_ch_man = map(m_in.channels, m_out.channels);
	}
	virtual void apply(const mp_audio *in) = 0;
	virtual void setScaler(bool on, double scale) = 0;
	virtual void setOutput(mp_audio *output) = 0;
protected:
	virtual void configured() = 0;
	AudioMixer(const AudioFormat &in, const AudioFormat &out, ClippingMethod clip)
	: m_in(in), m_out(out), m_clip(clip) {  }

	AudioFormat m_in, m_out;
	struct LevelInfo { LevelInfo(int frames = 0): frames(frames) {} int frames = 0; double level = 0.0; };
	double m_delay = 0.0, m_scale = 1.0;
	float m_gain[MP_NUM_CHANNELS], m_amp[MP_NUM_CHANNELS];
	ClippingMethod m_clip;
	QVector<QLinkedList<LevelInfo>> m_inputLevelHistory;
	QVector<LevelInfo> m_inputLevels;
	bool m_scaleChanged = false;
	bool m_normalizer = false, m_muted = false, m_updateChmap = false, m_updateFormat = false;
	QVector<typename QLinkedList<LevelInfo>::iterator> m_its;
	AudioNormalizerOption m_normalizerOption;
	std::array<int, MP_SPEAKER_ID_COUNT> m_ch_index_src, m_ch_index_dst;
	ChannelManipulation m_ch_man;
	ChannelLayoutMap m_map;
	mp_audio *m_output = nullptr;
};

#endif // AUDIOMIXER_HPP
