#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include <QtCore/QThread>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtCore/QVariant>
#include "mrl.hpp"
#include "mpmessage.hpp"
#include "global.hpp"
#include <QtCore/QStringList>

class QString;			class QVariant;
class VideoRenderer;
class AudioController;	struct MPContext;
class VideoFormat;		struct mp_cmd;

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


class PlayEngine : public QThread, public MpMessage {
	Q_OBJECT
public:
	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	struct Cmd {
		enum Type {Unknown = 0, Quit = 1, Load = 2, Stop = 4, VideoUpdate = 8, Volume = 16, Break = Quit | Load | Stop};
		Cmd() {}
		Cmd(Type type): type(type) {}
		Cmd(Type type, const QVariant &var): type(type), var(var) {}
		Type type = Unknown;
		QVariant var = {};
	};
	~PlayEngine();
	void tellmp(const QString &cmd);
	void tellmp(const QString &cmd, const QVariant &arg);
	void tellmp(const QString &cmd, const QVariant &arg1, const QVariant &arg2);
	void tellmp(const QString &cmd, const QVariant &arg1, const QVariant &arg2, const QVariant &arg3);
	void tellmp(const QString &cmd, const QStringList &args);

	VideoRenderer &renderer() const;

	MPContext *context() const;
	void setMrl(const Mrl &mrl, bool play);
	int position() const;
	State state() const;
	bool isSeekable() const;
	void setSpeed(double speed);
	double speed() const;
	bool hasVideo() const;
	int duration() const;
	bool atEnd() const;
	Mrl mrl() const;
	bool isPlaying() const {return state() == State::Playing;}
	bool isPaused() const {return state() == State::Paused;}
	bool isStopped() const {return state() == State::Stopped;}
	int currentTitleId() const;
	int currentChapterId() const;
	int currentSpuId() const;
	QStringList chapters() const;
	void setDvdDevice(const QString &name);
	QMap<int, DvdInfo::Title> titles() const;
	StreamList spus() const;
	void setCurrentTitle(int id);
	void setCurrentChapter(int id);
	void setCurrentSpu(int id);
	static PlayEngine &get() {return *obj;}
	const VideoFormat &videoFormat() const;
	void enqueue(Cmd *cmd);
	bool isMenu() const;
	QString volumeName() const;
	QString mediaName() const;
	bool isInitialized() const;
	bool isFrameDroppingEnabled() const;
	static void msleep(unsigned long msec) {QThread::msleep(msec);}
	static void usleep(unsigned long usec) {QThread::usleep(usec);}
	void play(int time);
	bool isHardwareAccelerated() const;
public slots:
	void quitRunning();
	void play();
	void stop();
	void pause();
	void seek(int pos);
	void relativeSeek(int pos);
	void setFrameDroppingEnabled(bool enabled);
signals:
	void initialized();
	void finalized();
	void started(Mrl mrl);
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(State state, State old);
	void seekableChanged(bool seekable);
	void durationChanged(int duration);
	void aboutToPlay();
	void aboutToOpen();
private slots:
	void emitTick();
private:
	void clear();
	struct Context;
	static int runCmd(MPContext *mpctx, mp_cmd *mpcmd);
	static void onPausedChanged(MPContext *mpctx);
	static int updateVideo(struct MPContext *mpctx);
	static mp_cmd *waitCmd(MPContext *mpctx, int timeout, int peek);
	int processCmds();
	int idle();
	void run_mp_cmd(const char *str);
//	Cmd *dequeue(int time = -1);
	friend void plug(PlayEngine *engine, AudioController *audio);
	friend void unplug(PlayEngine *engine, AudioController *audio);
	void load();
	int getStartTime() const;
	PlayEngine();
	void updateState(State state);
	void run();
	bool parse(const Id &id);
	bool parse(const QString &line);
	struct Data;
	static PlayEngine *obj;
	friend int main(int argc, char **argv);
	Data *d;
	AudioController *m_audio = nullptr;
};

#endif // PLAYENGINE_HPP
