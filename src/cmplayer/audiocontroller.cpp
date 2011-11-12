#include "audiocontroller.hpp"
#include <QtCore/QMetaType>
#include <QtCore/QVector>
#include "avmisc.hpp"
#include <cmath>
#include <QtCore/QDebug>

class AudioController::Volume {
public:
	Volume() {
		m_smoothness = 50;
		m_volume = 100;
		m_amp = 1.0;
		m_muted = false;
		m_normalized = false;
		m_targetGain = 0.75f;
		m_gainAvg = 0.0f;
	}
	~Volume() {}
	struct PrevInfo {
		PrevInfo(): value(0), samples(0) {}
		float value;
		int samples;
	};

	struct Channel {
		Channel(): gain(1.0f), last(0) {}
		float gain;
		QVector<PrevInfo> prev;
		int last;
	};
	static constexpr float silence = 1e-4;
	void init(const AudioFormat &format) {
		m_channels = QVector<Channel>(format.channels);
	}
	void calculateGain(AudioBuffer *in) {
		const int samples = in->samples;
		const int channels = m_channels.size();
		float gainAvg = 0.0f;
		for (int c=0; c<channels; ++c) {
			Channel &ch = m_channels[c];
			if (m_smoothness != ch.prev.size()) {
				ch.prev = QVector<PrevInfo>(m_smoothness);
				ch.last = 0;
			}
			float max = 0.0;
			int total = 0;
			for (int i=0; i<ch.prev.size(); ++i) {
				const PrevInfo &info = ch.prev[i];
				max += info.value*info.samples;
				total += info.samples;
			}
			max /= (float)total;
			ch.gain = 1.0f;
			if (max > silence)
				ch.gain = qBound(0.f, m_targetGain/max, 5.f);
			gainAvg += ch.gain;

			max = 0.0;
			float *buffer = ((float*)in->data) + c;
			for (int s=0; s<samples; ++s) {
				const float v = std::fabs(*buffer);
				if (v > max)
					max = v;
				buffer += channels;
			}
			ch.prev[ch.last].value = max;
			ch.prev[ch.last].samples = samples;
			ch.last = (ch.last + 1)%ch.prev.size();
		}
		m_gainAvg = gainAvg/(float)channels;
	}
	AudioBuffer *process(AudioBuffer *in) {
		float *buffer = (float*)in->data;
		const int samples = in->samples;
		const float coef = coefficient();
		const int channels = m_channels.size();
		if (m_muted || !m_normalized) {
			for(int i=0; i<samples; ++i) {
				for(int c=0; c<channels; ++c)
					buffer[c] *= coef;
				buffer += channels;
			}
		} else {
			calculateGain(in);
			for(int s=0; s<samples; ++s) {
				for(int c=0; c<channels; ++c) {
					buffer[c] *= m_channels[c].gain;
					buffer[c] *= coef;
				}
				buffer += channels;
			}
		}
		return 0;
	}
	float gain() const {return m_gainAvg;}
	void setMuted(bool muted) {m_muted = muted;}
	void setVolume(int volume) {m_volume = volume;}
	void setAmp(double amp) {m_amp = amp;}
	int volume() const {return m_volume;}
	bool isMuted() const {return m_muted;}
	double amp() const {return m_amp;}
	void setNormalized(bool norm) {m_normalized = norm;}
	bool isNormalized() const {return m_normalized;}
	void setTargetGain(float gain) {m_targetGain = gain;}
	float targetGain() const {return m_targetGain;}
	float gain(int ch) const {return (0 <= ch && ch < m_channels.size()) ? m_channels[ch].gain : 0.f;}
	void setSmoothness(int smooth) {m_smoothness = smooth;}
private:
	float coefficient() const {return m_muted ? .0 : (m_amp*m_volume)/100.;}
	QVector<Channel> m_channels;
	int m_volume;
	double m_amp;
	bool m_muted;
	bool m_normalized;
	float m_targetGain, m_gainAvg;
	int m_smoothness;
};

struct AudioController::Data {
	AudioUtil *util;
	AudioFormat format;
	Volume volume;
};

AudioController::AudioController(AudioUtil *util)
: d(new Data) {
	qRegisterMetaType<AudioFormat>("AudioFormat");
	d->util = util;
}

AudioController::~AudioController() {
	delete d;
}

void AudioController::prepare(const AudioFormat *format) {
	d->format = *format;
	d->volume.init(d->format);
	emit formatChanged(d->format);
}

AudioBuffer *AudioController::process(AudioBuffer *in) {
	return d->volume.process(in);
}

int AudioController::volume() const {
	return d->volume.volume();
}

bool AudioController::isMuted() const {
	return d->volume.isMuted();
}

void AudioController::setVolume(int volume) {
	volume = qBound(0, volume, 100);
	if (d->volume.volume() != volume) {
		d->volume.setVolume(volume);
		emit volumeChanged(volume);
	}
}

void AudioController::setMuted(bool muted) {
	if (d->volume.isMuted() != muted) {
		d->volume.setMuted(muted);
		emit mutedChanged(muted);
	}
}

void AudioController::setPreAmp(double amp) {
	amp = qFuzzyCompare(amp, 1.0) ? 1.0 : qBound(0.0, amp, 10.0);
	if (!qFuzzyCompare(d->volume.amp(), amp))
		d->volume.setAmp(amp);
}

double AudioController::preAmp() const {
	return d->volume.amp();
}

void AudioController::setVolumeNormalized(bool norm) {
	if (d->volume.isNormalized() != norm) {
		d->volume.setNormalized(norm);
		emit volumeNormalizedChanged(norm);
	}
}

bool AudioController::isVolumeNormalized() const {
	return d->volume.isNormalized();
}

void AudioController::setTempoScaled(bool scaled) {
	if (d->util->scaletempo_enabled != scaled)
		emit tempoScaledChanged((d->util->scaletempo_enabled = scaled));
}

bool AudioController::isTempoScaled() const {
	return d->util->scaletempo_enabled != 0;
}

double AudioController::gain(int ch) const {
	return d->volume.gain(ch);
}

double AudioController::gain() const {
	return d->volume.gain();
}

double AudioController::targetGain() const {
	return d->volume.targetGain();
}

void AudioController::setTargetGain(double gain) {
	d->volume.setTargetGain(gain);
}

void AudioController::setNormalizerSmoothness(int smooth) {
	d->volume.setSmoothness(smooth);
}
