#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

#include <QtCore/QObject>
#include <QtCore/QStringBuilder>
#include "mpmessage.hpp"

class PlayEngine;		class AudioFormat;
class AudioController;	class mixer;

void plug(PlayEngine *engine, AudioController *audio);
void unplug(PlayEngine *engine, AudioController *audio);

class AudioController : public QObject, public MpMessage {
	Q_OBJECT
public:
	AudioController();
	~AudioController();
	int volume() const;
	bool isMuted() const;
	void setPreAmp(double amp);
	double preAmp() const;
	bool isVolumeNormalized() const;
	bool isTempoScaled() const;
	double realVolume() const;
	StreamList streams() const;
	void setCurrentStream(int id) const;
	int currentStreamId() const;
	struct mixer *mixer() const;
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
private slots:
	void onAboutToPlay();
	void onAboutToOpen();
private:
	bool parse(const Id &id);
	bool isRunning() const;
	void apply();
	friend void plug(PlayEngine *engine, AudioController *audio);
	friend void unplug(PlayEngine *engine, AudioController *audio);
	struct Data;
	Data *d;
};

#endif // AUDIOCONTROLLER_HPP
