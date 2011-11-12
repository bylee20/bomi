#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

#include <QtCore/QObject>

class PlayEngine;		class AudioFormat;
class AudioBuffer;		class AudioUtil;

class AudioController : public QObject {
	Q_OBJECT
public:
	~AudioController();
	int volume() const;
	bool isMuted() const;
	void setPreAmp(double amp);
	double preAmp() const;
	bool isVolumeNormalized() const;
	bool isTempoScaled() const;
	double gain(int ch) const;
	double gain() const;
	double targetGain() const;
	void setTargetGain(double gain);
	void setNormalizerSmoothness(int smooth);
public slots:
	void setVolumeNormalized(bool norm);
	void setVolume(int volume);
	void setTempoScaled(bool scaled);
	void setMuted(bool muted);
signals:
	void formatChanged(const AudioFormat &format);
	void volumeChanged(int volume);
	void mutedChanged(bool muted);
	void volumeNormalizedChanged(bool norm);
	void tempoScaledChanged(bool scaled);
private:
	class Volume;
	void prepare(const AudioFormat *format);
	AudioController(AudioUtil *util);
	AudioBuffer *process(AudioBuffer *in);
	friend class LibVLC;
	struct Data;
	Data *d;
};

#endif // AUDIOCONTROLLER_HPP
