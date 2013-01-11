#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "mpmessage.hpp"
#include "global.hpp"

class VideoRendererItem;	struct MPContext;
class VideoFormat;			struct mp_cmd;

struct DvdInfo {
	QString volume = {};
	struct Title {
		QString name = {};
		QStringList chapters = {};
	};
	QMap<int, Title> titles = {};
	void clear() {titles.clear(); volume.clear();}
};

struct Track {
	Track() {}
	Track(const QString &lang, const QString &title): m_title(title), m_lang(lang) {}
	QString name() const {return m_title % ' ' % m_lang;}
	QString title() const {return m_title;}
	QString language() const {return m_lang;}
private:
	QString m_title = {}, m_lang = {};
};

typedef QList<Track> TrackList;
typedef QLinkedList<QString> FilterList;

class PlayEngine : public QThread, public MpMessage {
	Q_OBJECT
public:
	struct Cmd {
		enum Type {Unknown = 0, Quit = 1, Load = 2, Stop = 4, Volume = 16, Break = Quit | Load | Stop};
		Cmd() {} Cmd(Type type): type(type) {} Cmd(Type type, const QVariant &var): type(type), var(var) {}
		Type type = Unknown; QVariant var;
	};
	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	~PlayEngine();
	MPContext *context() const;
	int position() const;

	bool hasVideo() const;
	bool atEnd() const;
	bool isSeekable() const;
	bool isPlaying() const {return state() == EnginePlaying;}
	bool isPaused() const {return state() == EnginePaused;}
	bool isStopped() const {return state() == EngineStopped;}
	bool isFinished() const {return state() == EngineFinished;}
	EngineState state() const {return m_state;}
	double speed() const {return m_speed;}
	int duration() const {return m_duration;}
	Mrl mrl() const {return m_mrl;}
	int currentTitle() const {return m_isInDvdMenu ? 0 : m_title;}
	QMap<int, DvdInfo::Title> titles() const {return m_dvd.titles;}
	int currentChapter() const;
	int currentSubtitleStream() const;
	QStringList chapters() const {auto it = m_dvd.titles.find(currentTitle()); return it != m_dvd.titles.end() ? it->chapters : QStringList();}
	StreamList subtitleStreams() const {return m_subtitleStreams;}
	void setCurrentTitle(int id) {if (id > 0) tellmp("switch_title", id); else tellmp("dvdnav menu");}
	void setCurrentChapter(int id) {tellmp("seek_chapter", id, 1);}
	void setCurrentSubtitleStream(int id) {tellmp("sub_demux", id);}
	static PlayEngine &get() {return *obj;}
	void enqueue(Cmd *cmd);
	bool isInDvdMenu() const {return m_isInDvdMenu;}
	QString mediaName() const;
	bool isInitialized() const;
	bool frameDrop() const {return m_framedrop;}
	void setFrameDrop(bool on) {tellmp("frame_drop", (m_framedrop = on) ? 1 : 0);}
	static void msleep(unsigned long msec) {QThread::msleep(msec);}
	static void usleep(unsigned long usec) {QThread::usleep(usec);}
	void play(int time);
	bool usingHwAcc() const;
	void clearQueue();
	void setMrl(const Mrl &mrl, int start, bool play);

	void setSpeed(double speed);
	void setAudioFilter(const QString &af, bool on);
	bool hasAudioFilter(const QString &af) const {return m_af.contains(af);}
	double volumeNormalizer() const;

	int volume() const {return m_volume;}
	bool isMuted() const {return m_muted;}
	double preamp() const {return m_preamp;}

	double mpVolume() const {return qBound(0.0, m_preamp*m_volume, 1000.0)/10.0;}

	double fps() const;

	VideoRendererItem *videoRenderer() const {return m_renderer;}

	VideoFormat videoFormat() const;
	StreamList audioStreams() const {return m_audiosStreams;}
	void setCurrentAudioStream(int id) {tellmp("switch_audio", id);}
	int currentAudioStream() const;


	StreamList videoStreams() const {return m_videoStreams;}
	void setCurrentVideoStream(int id) {tellmp("switch_video", id);}
	int currentVideoStream() const;

public slots:
	void setVolume(int volume) {
		if (_Change(m_volume, qBound(0, volume, 100))) {
			enqueue(new Cmd(Cmd::Volume, mpVolume())); emit volumeChanged(m_volume);
		}
	}
	void setMuted(bool muted) {
		if (_Change(m_muted, muted)) {
			tellmp("mute", (int)m_muted); emit mutedChanged(m_muted);
		}
	}
	void setPreamp(double preamp) {
		if (_ChangeF(m_preamp, qFuzzyCompare(preamp, 1.0) ? 1.0 : qBound(0.0, preamp, 10.0))) {
			enqueue(new Cmd(Cmd::Volume, mpVolume())); emit preampChanged(m_preamp);
		}
	}
//	void setVolumeNormalized(bool norm);
//	void setTempoScaled(bool scaled);

	void setVideoRenderer(VideoRendererItem *renderer);
public slots:
	void quitRunning();
	void play();
	void stop() {enqueue(new Cmd(Cmd::Stop));}
	void pause() {if (!isPaused()) tellmp("pause");}

	void seek(int pos) {tellmp("seek", (double)pos/1000.0, 2);}
	void relativeSeek(int pos) {tellmp("seek", (double)pos/1000.0, 0);}
signals:
	void initialized();
	void finalized();
	void started(Mrl mrl);
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(EngineState newState, EngineState oldState);
	void seekableChanged(bool seekable);
	void durationChanged(int duration);
	void aboutToPlay();
	void aboutToOpen();
	void volumeChanged(int volume);
	void preampChanged(double preamp);
	void mutedChanged(bool muted);
	void audioFilterChanged(const QString &af, bool on);
	void videoFormatChanged(const VideoFormat &format);
private:
	void tellmp(const QString &cmd);
	void tellmp(const QString &cmd, const QVariant &arg) {tellmp(cmd % _L(' ') % arg.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2, const QVariant &a3) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString() % ' ' % a3.toString());}
	template<template <typename> class T> void tellmp(const QString &cmd, const T<QString> &args) {QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);}
	void updateVideoAspect(double ratio);
	void setState(EngineState state) {if (m_state != state) {const auto prev = m_state; m_state = state; emit stateChanged(m_state, prev);}}
	void clear();
	struct Context;
	static int runCmd(MPContext *mpctx, mp_cmd *mpcmd);
	static void onPausedChanged(MPContext *mpctx);
	static mp_cmd *waitCmd(MPContext *mpctx, int timeout, int peek);
	int processCmds();
	int idle();
	void run_mp_cmd(const char *str);
	PlayEngine();
	void run();
	bool parse(const Id &id);
	bool parse(const QString &line);
	struct Data;
	static PlayEngine *obj;
	friend int main(int argc, char **argv);
	Data *d;
	VideoRendererItem *m_renderer = nullptr;
//	friend class VideoOutput;
	Mrl m_mrl;
	int m_duration = 0;
	EngineState m_state = EngineStopped;
	double m_speed = 1.0;
	bool m_framedrop = false;
	bool m_isInDvdMenu = false;
	StreamList m_subtitleStreams, m_audiosStreams, m_videoStreams;
	int m_title = 0;
	DvdInfo m_dvd;

	FilterList m_af;
	int m_volume = 100;
	double m_preamp = 1.0;
	bool m_muted = false;


};

#endif // PLAYENGINE_HPP
