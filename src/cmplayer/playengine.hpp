#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "mpmessage.hpp"
#include "global.hpp"
#include <functional>

class VideoRendererItem;	struct MPContext;
class VideoFormat;			struct mp_cmd;
class PlaylistModel;		class Playlist;
typedef std::function<int(const Mrl&)> GetStartTime;

struct DvdInfo {
	struct Title {
		QString name() const {return m_name;}
		int id() const {return m_id;}
		int m_id = 0;
		int number = 0, chapters = 0, angles = 0, length = 0;
	private:
		friend class PlayEngine;
		QString m_name;
	};
	void clear() {titles.clear(); titles.clear(); id.clear();}
	QMap<int, Title> titles;
	QString volume;
	QString id = 0;
};

struct Chapter {
	QString name() const {return m_name;}
	int id() const {return m_id;}
private:
	friend class PlayEngine;
	QString m_name;
	int m_id = 0;
};

typedef QLinkedList<QString> FilterList;

class PlayEngine : public QThread, public MpMessage {
	Q_OBJECT
public:
	struct Context;
	static void msleep(unsigned long msec) {QThread::msleep(msec);}
	static void usleep(unsigned long usec) {QThread::usleep(usec);}
	PlayEngine();
	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	~PlayEngine();
	MPContext *context() const;
	int position() const;
	int duration() const {return m_duration;}
	void setPlaylist(const Playlist &playlist);
	Mrl mrl() const;
	bool atEnd() const;
	bool isSeekable() const;
	void setHwAccCodecs(const QList<int> &codecs);
	bool isPlaying() const {return state() == EnginePlaying;}
	bool isPaused() const {return state() == EnginePaused;}
	bool isStopped() const {return state() == EngineStopped;}
	bool isFinished() const {return state() == EngineFinished;}
	bool isInitialized() const;
	double speed() const {return m_speed;}
	EngineState state() const {return m_state;}
	void load(const Mrl &mrl, int start = -1);
	void load(const Mrl &mrl, bool play);
	void setSpeed(double speed) {if (_ChangeZ(m_speed, speed)) {setmp("speed", speed);}}
	const DvdInfo &dvd() const {return m_dvd;}
	int currentDvdTitle() const {return m_title;}
	int currentChapter() const;
	QList<Chapter> chapters() const {return m_chapters;}
	int currentSubtitleStream() const;
	StreamList subtitleStreams() const {return m_subtitleStreams;}
	void setCurrentSubtitleStream(int id);
	void setCurrentDvdTitle(int id);
	void setCurrentChapter(int id) {setmp("chapter", id);}
	bool hasVideo() const;
	bool frameDrop() const {return m_framedrop;}
	bool isHwAccActivated() const;
	void setFrameDrop(bool on) {tellmp("frame_drop", (m_framedrop = on) ? 1 : 0);}
	void setVolumeNormalized(bool on);
	void setTempoScaled(bool on);
	bool isVolumeNormalized() const;
	bool isTempoScaled() const;

//	void setAudioFilter(const QString &af, bool on);
//	bool hasAudioFilter(const QString &af) const {return m_af.contains(af);}
	double fps() const;
	double videoAspectRatio() const;
	VideoRendererItem *videoRenderer() const {return m_renderer;}
	VideoFormat videoFormat() const;
	StreamList videoStreams() const {return m_videoStreams;}
	void setCurrentVideoStream(int id) {setmp("video", id);}
	int currentVideoStream() const;
	void setGetStartTimeFunction(const GetStartTime &func);
	const PlaylistModel &playlist() const;
	PlaylistModel &playlist();

	int volume() const {return m_volume;}
	int currentAudioStream() const;
	bool isMuted() const {return m_muted;}
	double volumeNormalizer() const;
	double preamp() const {return m_preamp;}
	StreamList audioStreams() const {return m_audioStreams;}
	void setCurrentAudioStream(int id) {setmp("audio", id);}
public slots:
	void setVolume(int volume) {if (_Change(m_volume, qBound(0, volume, 100))) {setMpVolume(); emit volumeChanged(m_volume);}}
	void setPreamp(double preamp) {if (_ChangeZ(m_preamp, qBound(0.0, preamp, 10.0))) {	setMpVolume();	emit preampChanged(m_preamp);}}
	void setMuted(bool muted) {if (_Change(m_muted, muted)) {setmp("mute", m_muted); emit mutedChanged(m_muted);}}
	void setVideoRenderer(VideoRendererItem *renderer);
	void play();
	void stop();
	void quit();
	void reload();
	void pause() {if (!isPaused()) tellmp("pause");}
	void seek(int pos) {tellmp("seek", (double)pos/1000.0, 2);}
	void relativeSeek(int pos) {tellmp("seek", (double)pos/1000.0, 0);}
	void runCommand(mp_cmd *cmd);
signals:
	void tempoScaledChanged(bool on);
	void volumeNormalizedChanged(bool on);
	void started(Mrl mrl);
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(EngineState state);
	void seekableChanged(bool seekable);
	void durationChanged(int duration);
	void volumeChanged(int volume);
	void preampChanged(double preamp);
	void mutedChanged(bool muted);
	void videoFormatChanged(const VideoFormat &format);
	void videoAspectRatioChanged(double ratio);
	void dvdInfoChanged();
private:
	static void onPausedChanged(MPContext *mpctx);
	static void onPlayStarted(MPContext *mpctx);
	bool load(int row, int start = -1);
	void play(int time);
	void clear();
	void setmp(const char *name, int value);
	void setmp(const char *name, float value);
	void setmp(const char *name, double value) {setmp(name, (float)value);}
	void setMpVolume() {setmp("volume", mpVolume());}
	float mpVolume() const {return qBound(0.0, m_preamp*m_volume, 1000.0)/10.0;}
	QPoint mapToFrameFromTop(const QPoint &pos);
	void tellmp(const QString &cmd);
	void tellmp(const QString &cmd, const QVariant &arg) {tellmp(cmd % _L(' ') % arg.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2, const QVariant &a3) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString() % ' ' % a3.toString());}
	template<template <typename> class T> void tellmp(const QString &cmd, const T<QString> &args) {QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);}
	void setVideoAspect(double ratio);
	void setState(EngineState state);
	void run();
	bool parse(const Id &id);
	bool parse(const QString &line);
	void customEvent(QEvent *event);
	int m_duration = 0, m_title = 0, m_volume = 100, m_subId = -1;
	EngineState m_state = EngineStopped;
	double m_speed = 1.0, m_preamp = 1.0;
	bool m_framedrop = false, m_muted = false;
	StreamList m_subtitleStreams, m_audioStreams, m_videoStreams;
	VideoRendererItem *m_renderer = nullptr;
	DvdInfo m_dvd;
	QList<Chapter> m_chapters;
	struct Data; Data *d;
};

#endif // PLAYENGINE_HPP
