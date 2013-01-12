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
	static PlayEngine &get() {return *obj;}
	static void msleep(unsigned long msec) {QThread::msleep(msec);}
	static void usleep(unsigned long usec) {QThread::usleep(usec);}

	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	~PlayEngine();
	MPContext *context() const;
	int position() const;
	int duration() const {return m_duration;}
	Mrl mrl() const {return m_mrl;}
	bool atEnd() const;
	bool isSeekable() const;
	bool isPlaying() const {return state() == EnginePlaying;}
	bool isPaused() const {return state() == EnginePaused;}
	bool isStopped() const {return state() == EngineStopped;}
	bool isFinished() const {return state() == EngineFinished;}
	bool isInitialized() const;
	double speed() const {return m_speed;}
	QString mediaName() const;
	EngineState state() const {return m_state;}
	void play(int time);
	void setMrl(const Mrl &mrl, int start, bool play);
	void setSpeed(double speed);
	bool handleMousePressed(const QPoint &pos);
	bool handleMouseMoved(const QPoint &pos);

	bool isInDvdMenu() const {return m_isInDvdMenu;}
	int currentTitle() const {return m_isInDvdMenu ? 0 : m_title;}
	int currentChapter() const;
	QMap<int, DvdInfo::Title> titles() const {return m_dvd.titles;}
	QStringList chapters() const {auto it = m_dvd.titles.find(currentTitle()); return it != m_dvd.titles.end() ? it->chapters : QStringList();}
	int currentSubtitleStream() const;
	StreamList subtitleStreams() const {return m_subtitleStreams;}
	void setCurrentSubtitleStream(int id) {tellmp("sub_demux", id);}
	void setCurrentTitle(int id) {if (id > 0) tellmp("switch_title", id); else tellmp("dvdnav menu");}
	void setCurrentChapter(int id) {tellmp("seek_chapter", id, 1);}

	bool hasVideo() const;
	bool frameDrop() const {return m_framedrop;}
	bool usingHwAcc() const;
	void setFrameDrop(bool on) {tellmp("frame_drop", (m_framedrop = on) ? 1 : 0);}
	void setAudioFilter(const QString &af, bool on);
	bool hasAudioFilter(const QString &af) const {return m_af.contains(af);}
	double fps() const;
	double videoAspectRatio() const;
	VideoRendererItem *videoRenderer() const {return m_renderer;}
	VideoFormat videoFormat() const;
	StreamList videoStreams() const {return m_videoStreams;}
	void setCurrentVideoStream(int id) {tellmp("switch_video", id);}
	int currentVideoStream() const;

	int volume() const {return m_volume;}
	int currentAudioStream() const;
	bool isMuted() const {return m_muted;}
	double volumeNormalizer() const;
	double preamp() const {return m_preamp;}
	StreamList audioStreams() const {return m_audiosStreams;}
	void setCurrentAudioStream(int id) {tellmp("switch_audio", id);}
public slots:
	void setVolume(int volume) {
		if (_Change(m_volume, qBound(0, volume, 100))) {
//			enqueue(new Cmd(Cmd::Volume, mpVolume()));
			tellmp("volume", mpVolume(), 1);
			emit volumeChanged(m_volume);
		}
	}
	void setMuted(bool muted) {
		if (_Change(m_muted, muted)) {
			tellmp("mute", (int)m_muted); emit mutedChanged(m_muted);
		}
	}
	void setPreamp(double preamp) {
		if (_ChangeF(m_preamp, qFuzzyCompare(preamp, 1.0) ? 1.0 : qBound(0.0, preamp, 10.0))) {
			tellmp("volume", mpVolume(), 1);
//			enqueue(new Cmd(Cmd::Volume, mpVolume()));
			emit preampChanged(m_preamp);
		}
	}
	void setVideoRenderer(VideoRendererItem *renderer);
	void quitRunning();
	void play();
	void stop();
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
	void videoAspectRatioChanged(double ratio);
private:
	struct Cmd;	struct Context;
	static int runCmd(MPContext *mpctx, mp_cmd *mpcmd);
	static void onPausedChanged(MPContext *mpctx);
	static mp_cmd *waitCmd(MPContext *mpctx, int timeout, int peek);
	void clear();
	void enqueue(Cmd *cmd);
	void clearQueue();
	double mpVolume() const {return qBound(0.0, m_preamp*m_volume, 1000.0)/10.0;}
	QPoint mapToFrameFromTop(const QPoint &pos);
	void tellmp(const QString &cmd);
	void tellmp(const QString &cmd, const QVariant &arg) {tellmp(cmd % _L(' ') % arg.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2, const QVariant &a3) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString() % ' ' % a3.toString());}
	template<template <typename> class T> void tellmp(const QString &cmd, const T<QString> &args) {QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);}
	void setVideoAspect(double ratio);
	void setState(EngineState state) {if (m_state != state) {const auto prev = m_state; m_state = state; emit stateChanged(m_state, prev);}}
	int processCmds();
	int idle();
	void run_mp_cmd(const char *str);
	PlayEngine();
	void run();
	bool parse(const Id &id);
	bool parse(const QString &line);
	static PlayEngine *obj;
	friend int main(int argc, char **argv);
	int m_duration = 0, m_title = 0, m_volume = 100;
	EngineState m_state = EngineStopped;
	double m_speed = 1.0, m_preamp = 1.0;
	bool m_framedrop = false, m_isInDvdMenu = false,m_muted = false;
	StreamList m_subtitleStreams, m_audiosStreams, m_videoStreams;
	VideoRendererItem *m_renderer = nullptr;
	DvdInfo m_dvd;	FilterList m_af;	Mrl m_mrl;
	struct Data; Data *d;
};

#endif // PLAYENGINE_HPP
