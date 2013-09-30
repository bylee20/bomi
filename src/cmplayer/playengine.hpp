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
class DeintOption;
enum class AudioDriver;		enum class ClippingMethod;

typedef std::function<int(const Mrl&)> GetMrlInt;

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
	void clear() {titles.clear(); titles.clear(); }
	QMap<int, Title> titles;
	QString volume;
};

struct Chapter {
	QString name() const {return m_name;}
	int id() const {return m_id;}
	bool operator == (const Chapter &rhs) const {
		return m_id == rhs.m_id && m_name == rhs.m_name;
	}
private:
	friend class PlayEngine;
	QString m_name;
	int m_id = 0;
};

typedef QVector<Chapter> ChapterList;

typedef QLinkedList<QString> FilterList;

class PlayEngine : public QThread, public MpMessage {
	Q_OBJECT
public:
	static void msleep(unsigned long msec) {QThread::msleep(msec);}
	static void usleep(unsigned long usec) {QThread::usleep(usec);}
	PlayEngine();
	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	~PlayEngine();
	MPContext *context() const;
	int position() const;
	int startPosition() const { return m_startPos; }
	int endPosition() const { return m_startPos + m_duration; }
	void setImageDuration(int duration) {m_imgDuration = duration;}
	int duration() const {return m_imgMode ? m_imgDuration : m_duration;}
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
	void setSpeed(double speed) {if (_ChangeZ(m_speed, speed)) {setmp("speed", speed); emit speedChanged(m_speed);}}
	const DvdInfo &dvd() const {return m_dvd;}
	int currentDvdTitle() const {return m_title;}
	int currentChapter() const;
	ChapterList chapters() const {return m_chapters;}
	int currentSubtitleStream() const;
	StreamList subtitleStreams() const {return m_subtitleStreams;}
	void setCurrentSubtitleStream(int id);
	void setCurrentDvdTitle(int id);
	void setCurrentChapter(int id) {setmp("chapter", id);}
	bool hasVideo() const;
	bool frameDrop() const {return m_framedrop;}
	bool isHwAccActivated() const;
	void setVolumeNormalizerActivated(bool on);
	void setTempoScalerActivated(bool on);
	bool isVolumeNormalized() const;
	bool isTempoScaled() const;
	double fps() const;
	VideoRendererItem *videoRenderer() const {return m_renderer;}
	VideoFormat videoFormat() const;
	StreamList videoStreams() const {return m_videoStreams;}
	void setCurrentVideoStream(int id) {setmp("video", id);}
	int currentVideoStream() const;
	void setGetStartTimeFunction(const GetMrlInt &func);
	void setGetCacheFunction(const GetMrlInt &func);
	void setAudioSync(int sync) {if (_Change(m_audioSync, sync)) setmp("audio-delay", (float)(sync*0.001));}
	int audioSync() const {return m_audioSync;}
	const PlaylistModel &playlist() const;
	PlaylistModel &playlist();

	int volume() const {return m_volume;}
	int currentAudioStream() const;
	bool isMuted() const {return m_muted;}
	double volumeNormalizer() const;
	double preamp() const {return m_preamp;}
	StreamList audioStreams() const {return m_audioStreams;}
	void setCurrentAudioStream(int id) {setmp("audio", id);}
	void setVolumeNormalizerOption(double length, double target, double silence, double min, double max);
	void addSubtitleStream(const QString &fileName, const QString &enc);
	void removeSubtitleStream(int id);
	void setSubtitleStreamsVisible(bool visible);
	bool isSubtitleStreamsVisible() const {return m_subtitleStreamsVisible;}
	void setVideoFilters(const QString &vfs);
	void setDeintOptions(const DeintOption &sw, const DeintOption &hw);
	void setDeintEnabled(bool on);
	bool isDeintEanbled() const;
	void setAudioDriver(AudioDriver driver);
	AudioDriver preferredAudioDriver() const;
	AudioDriver audioDriver() const;
	void setClippingMethod(ClippingMethod method);
	void setMinimumCache(int playback, int seeking);
public slots:
	void redraw();
	void setVolume(int volume);
	void setPreamp(double preamp);
	void setMuted(bool muted);
	void setVideoRenderer(VideoRendererItem *renderer);
	void play();
	void stop();
	void quit();
	void reload();
	void pause();
	void unpause();
	void seek(int pos);
	void relativeSeek(int pos);
signals:
	void tempoScaledChanged(bool on);
	void volumeNormalizerActivatedChanged(bool on);
	void started(Mrl mrl);
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(EngineState state);
	void seekableChanged(bool seekable);
	void durationChanged(int duration);
	void startPositionChanged(int pos);
	void volumeChanged(int volume);
	void preampChanged(double preamp);
	void mutedChanged(bool muted);
	void videoFormatChanged(const VideoFormat &format);
	void audioStreamsChanged(const StreamList &streams);
	void videoStreamsChanged(const StreamList &streams);
	void subtitleStreamsChanged(const StreamList &streams);
	void chaptersChanged(const ChapterList &chapters);
	void dvdInfoChanged();
	void speedChanged(double speed);
private:
	static int mpCommandFilter(MPContext *mpctx, mp_cmd *cmd);
//	void finalizeGL();
	int playImage(const Mrl &mrl, int &terminated, int &duration);
	int playAudioVideo(const Mrl &mrl, int &terminated, int &duration);
	int currentTrackId(int type) const;
	bool load(int row, int start = -1);
	void play(int time);
	void clear();
	void setmp(const char *name, int value);
	void setmp(const char *name, float value);
	void setmp(const char *name, double value);
	void updateAudioLevel();
	QPoint mapToFrameFromTop(const QPoint &pos);
	void tellmp(const QString &cmd);
	void tellmp(const QString &cmd, const QVariant &arg) {tellmp(cmd % _L(' ') % arg.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString());}
	void tellmp(const QString &cmd, const QVariant &a1, const QVariant &a2, const QVariant &a3) {tellmp(cmd % ' ' % a1.toString() % ' ' % a2.toString() % ' ' % a3.toString());}
	template<template <typename> class T> void tellmp(const QString &cmd, const T<QString> &args) {QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);}
	void setState(EngineState state);
	void run();
	bool parse(const Id &id);
	bool parse(const QString &line);
	void customEvent(QEvent *event);
	int m_duration = 0, m_title = 0, m_volume = 100, m_audioSync = 0, m_startPos = 0;
	EngineState m_state = EngineStopped;
	double m_speed = 1.0, m_preamp = 1.0;
	bool m_framedrop = false, m_muted = false;
	StreamList m_subtitleStreams, m_audioStreams, m_videoStreams;
	VideoRendererItem *m_renderer = nullptr;
	DvdInfo m_dvd;
	ChapterList m_chapters;
	int m_imgDuration = 10000, m_imgPos = 0, m_imgSeek = 0, m_imgRelSeek = 0;
	struct Data; Data *d;
	bool m_imgMode = false, m_subtitleStreamsVisible = true;
};

#endif // PLAYENGINE_HPP
