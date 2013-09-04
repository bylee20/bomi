#ifndef AUDIOFILTER_HPP
#define AUDIOFILTER_HPP

#include "tmp.hpp"
extern "C" {
#include <audio/filter/af.h>
}

class AudioController;

class AudioFilter {
public:
	AudioFilter(int format): m_format(format) {}
	virtual ~AudioFilter() {}
	bool isCompatibleWith(const mp_audio *data) {
		return m_channels == data->channels.num
			&& m_samplerate == data->rate
			&& m_format == data->format;
	}
	virtual mp_audio* play(mp_audio* data) = 0;
	int fps() const { return m_samplerate; }
	int channels() const { return m_channels; }
	int format() const {return m_format;}
	int bps() const { return m_bps; }
	bool reconfigure(const mp_audio *data);
	bool prepare(const AudioController *ac, const mp_audio *data);
protected:
	virtual void prepareToPlay(const AudioController *ac, const mp_audio *data) = 0;
	virtual void reinitialize() = 0;
	int s2f(int samples) const { return samples/m_channels; }
	int f2s(int frames ) const { return frames *m_channels; }
	int s2b(int samples) const { return samples*m_bps; }
	int b2s(int bytes  ) const { return bytes / m_bps; }
private:
	friend class TempoScaler;
	friend class VolumeController;
	int m_channels = 0;
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
	double m_mul = 1.0, m_delay = 0.0;
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
	static VolumeController *create(int format);
	double gain() const { return m_gain; }
protected:
	double m_gain = 1.0;
	VolumeController(int format): AudioFilter(format) {}
};

#endif // AUDIOFILTER_HPP
