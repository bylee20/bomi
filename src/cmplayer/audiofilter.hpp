#ifndef AUDIOFILTER_HPP
#define AUDIOFILTER_HPP

#include "tmp.hpp"
#include "enums.hpp"
#include "channelmanipulation.hpp"
extern "C" {
#include <audio/filter/af.h>
}

class ChannelMix {
public:
	using SpeakerArray = QVector<mp_speaker_id>;
	ChannelMix() { }
	const SpeakerArray &sources(mp_speaker_id out) const { return m_mix[out]; }
	void set(mp_speaker_id dest, const SpeakerArray &src) { m_mix[dest] = src; }
	void set(mp_speaker_id dest, mp_speaker_id src) { m_mix[dest].resize(1); m_mix[dest][0] = src; }
	void add(mp_speaker_id dest, mp_speaker_id src) { m_mix[dest].append(src); }
private:
	std::array<SpeakerArray, MP_SPEAKER_ID_COUNT> m_mix;
	mp_chmap m_in, m_out;
};

class AudioController;

static inline bool operator == (const mp_chmap &lhs, const mp_chmap &rhs) {
	if (lhs.num != rhs.num)
		return false;
	for (int i=0; i<lhs.num; ++i)
		if (lhs.speaker[i] != rhs.speaker[i])
			return false;
	return true;
}

class AudioFilter {
public:
	AudioFilter(int format): m_format(format) {}
	virtual ~AudioFilter() {}
	bool isCompatibleWith(const mp_audio *data, const mp_audio *input) {
		return m_channels == data->channels && m_channels_in == input->channels
				&& m_samplerate == data->rate && m_format == data->format;
	}
	virtual mp_audio* play(mp_audio* data) = 0;
	int fps() const { return m_samplerate; }
	const mp_chmap &channels() const { return m_channels; }
	const mp_chmap &channels_in() const { return m_channels_in; }
	int format() const {return m_format;}
	int bps() const { return m_bps; }
	bool reconfigure(const mp_audio *data, const mp_audio *input = nullptr);
	bool prepare(const AudioController *ac, const mp_audio *data, const mp_audio *input);
	double multiplier() const {return m_mul;}
protected:
	virtual void prepareToPlay(const AudioController *ac, const mp_audio *data) = 0;
	virtual void reinitialize() = 0;
	int s2f(int samples) const { return samples/m_channels_in.num; }
	int f2s(int frames ) const { return frames *m_channels_in.num; }
	int s2b(int samples) const { return samples*m_bps; }
	int b2s(int bytes  ) const { return bytes / m_bps; }
	double m_mul = 1.0;
private:
	friend class TempoScaler;
	friend class VolumeController;
	mp_chmap m_channels, m_channels_in;
	int m_samplerate = 0;
	int m_format = AF_FORMAT_UNKNOWN;
	int m_bps = 1;
};

class TempoScaler : public AudioFilter {
public:
	static TempoScaler *create(int format);
	double multiplier() const {return m_mul;}
	double delay() const {return m_delay;}
protected:
	TempoScaler(int format): AudioFilter(format) {}
	double m_delay = 0.0;
};

struct NormalizerOption {
	bool operator == (const NormalizerOption &rhs) const {
		return silenceLevel == rhs.silenceLevel && targetLevel == rhs.targetLevel
			&& minimumGain == rhs.minimumGain && maximumGain == rhs.maximumGain
			&& bufferLengthInSeconds == rhs.bufferLengthInSeconds;
	}
	bool operator != (const NormalizerOption &rhs) const { return !operator==(rhs); }
	double gain(double level) {
		return (level > silenceLevel) ? qBound(minimumGain, targetLevel / level, maximumGain) : -1.0;
	}
	double silenceLevel = 0.0001, minimumGain = 0.1, maximumGain = 10.0, targetLevel = 0.07, bufferLengthInSeconds = 5.0;
};

class VolumeController : public AudioFilter {
public:
	static VolumeController *create(int format, ClippingMethod clip);
	double gain() const {
		double ret = 0.0; int count = 0;
		for (auto g : m_gain) {
			if (g > 1.0001) {
				ret += g;
				++count;
			}
		}
		return count > 0 ? ret/(double)count : 1.0;
	}
	ClippingMethod clippingMethod() const { return m_clip; }
	void setChannelLayoutMap(const ChannelLayoutMap &map) { m_map = map; }
protected:
	double m_gain[AF_NCH];
	ClippingMethod m_clip;
	ChannelLayoutMap m_map;
	VolumeController(int format, ClippingMethod clip): AudioFilter(format), m_clip(clip) {}
};

#endif // AUDIOFILTER_HPP
