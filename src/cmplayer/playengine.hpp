#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "global.hpp"
#include "mediamisc.hpp"
#include <functional>

class VideoRendererItem;	struct MPContext;
class VideoFormat;			struct mp_cmd;
class PlaylistModel;		class Playlist;
class DeintOption;			class ChannelLayoutMap;
enum class AudioDriver;		enum class ClippingMethod;
enum class DeintMethod;		enum class DeintMode;
enum class ChannelLayout;	struct SubtitleFileInfo;
struct mpv_event;

typedef QLinkedList<QString> FilterList;

struct StartInfo {
	StartInfo() {}
	StartInfo(const Mrl &mrl): mrl(mrl) {}
	Mrl mrl;
	int resume = -1;
	int cache = -1;
	bool isValid() const { return !mrl.isEmpty() && resume >= 0 && cache >= 0; }
};

class PlayEngine : public QObject {
	Q_OBJECT
	Q_ENUMS(State)
	Q_ENUMS(HardwareAcceleration)
	Q_PROPERTY(MediaInfoObject *media READ mediaInfo CONSTANT FINAL)
	Q_PROPERTY(AvInfoObject *audio READ audioInfo NOTIFY audioChanged)
	Q_PROPERTY(AvInfoObject *video READ videoInfo NOTIFY videoChanged)
	Q_PROPERTY(int begin READ begin NOTIFY beginChanged)
	Q_PROPERTY(int end READ end NOTIFY endChanged)
	Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(int time READ time NOTIFY tick)
	Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
	Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
	Q_PROPERTY(double volumeNormalizer READ volumeNormalizer)
	Q_PROPERTY(double avgsync READ avgsync)
	Q_PROPERTY(double avgfps READ avgfps)
	Q_PROPERTY(State state READ state NOTIFY stateChanged)
	Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
	Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
	Q_PROPERTY(double speed READ speed NOTIFY speedChanged)
	Q_PROPERTY(bool volumeNormalizerActivated READ isVolumeNormalizerActivated NOTIFY volumeNormalizerActivatedChanged)
	Q_PROPERTY(HardwareAcceleration hardwareAccelaration READ hwAcc NOTIFY hwaccChanged)
	Q_PROPERTY(double relativePosition READ relativePosition NOTIFY relativePositionChanged)
	Q_PROPERTY(QQuickItem *screen READ screen)
	Q_PROPERTY(qreal cache READ cache NOTIFY cacheChanged)
	Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
	Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
	Q_PROPERTY(ChapterInfoObject *chapter READ chapterInfo NOTIFY chaptersChanged)
	Q_PROPERTY(AudioTrackInfoObject *audioTrack READ audioTrackInfo NOTIFY audioStreamsChanged)
	Q_PROPERTY(SubtitleTrackInfoObject *subtitleTrack READ subtitleTrackInfo NOTIFY subtitleTrackInfoChanged)
public:
	enum State {Stopped = 1, Playing = 2, Paused = 4, Loading = 16, Error = 32, Buffering = 64, Running = Playing | Loading | Buffering };
	enum class HardwareAcceleration { Unavailable, Deactivated, Activated };
	enum DVDCmd { DVDMenu };
	PlayEngine();
	PlayEngine(const PlayEngine&) = delete;
	PlayEngine &operator = (const PlayEngine &) = delete;
	~PlayEngine();
	MPContext *context() const;
	int time() const;
	int begin() const;
	int end() const;
	void setImageDuration(int duration);
	int duration() const;
	Mrl mrl() const;
	bool isSeekable() const;
	void setHwAcc(int backend, const QList<int> &codecs);
	bool isRunning() const { return m_state & Running; }
	bool isPlaying() const {return m_state & Playing;}
	bool isPaused() const {return m_state & Paused;}
	bool isStopped() const {return m_state & Stopped;}
	double speed() const;
	State state() const { return m_state; }
//	void setCurrentMrl(const Mrl &mrl);
	void load(const StartInfo &info);
	const StartInfo &startInfo() const;
//	void play(int start, int cache);
//	void load(const MrlStartInfo &mrl, bool play);
	void setSpeed(double speed);
	const DvdInfo &dvd() const;
	int currentDvdTitle() const;
	int currentChapter() const;
	const ChapterList &chapters() const;
	int currentSubtitleStream() const;
	const StreamList &subtitleStreams() const;
	void setCurrentSubtitleStream(int id);
	void setCurrentDvdTitle(int id);
	void setCurrentChapter(int id);
	bool hasVideo() const;
	void setVolumeNormalizerActivated(bool on);
	void setTempoScalerActivated(bool on);
	bool isVolumeNormalizerActivated() const;
	bool isTempoScaled() const;
	double fps() const;
	VideoRendererItem *videoRenderer() const;
	VideoFormat videoFormat() const;
	const StreamList &videoStreams() const;
	void setCurrentVideoStream(int id);
	int currentVideoStream() const;
	void setAudioSync(int sync);
	int audioSync() const;
	const PlaylistModel &playlist() const;
	PlaylistModel &playlist();

	HardwareAcceleration hwAcc() const;
	int volume() const;
	int currentAudioStream() const;
	bool isMuted() const;
	double volumeNormalizer() const;
	double amp() const;
	const StreamList &audioStreams() const;
	void setCurrentAudioStream(int id);
	void setVolumeNormalizerOption(double length, double target, double silence, double min, double max);
	bool addSubtitleStream(const QString &fileName, const QString &enc);
	void removeSubtitleStream(int id);
	void setSubtitleStreamsVisible(bool visible);
	bool isSubtitleStreamsVisible() const;
	void setDeintOptions(const DeintOption &swdec, const DeintOption &hwdec);
	void setDeintMode(DeintMode mode);
	DeintMode deintMode() const;
	void setAudioDriver(AudioDriver driver);
	void setClippingMethod(ClippingMethod method);
	void setMinimumCache(int playback, int seeking);
	void run();
	void waitUntilTerminated();
	QThread *thread() const;
	QQuickItem *screen() const;
	MediaInfoObject *mediaInfo() const;
	AvInfoObject *audioInfo() const;
	AvInfoObject *videoInfo() const;
	double avgsync() const;
	double avgfps() const;
	QString stateText() const;
	static QString stateText(State state);
	double relativePosition() const { return (double)(time()-begin())/duration(); }
	Q_INVOKABLE double bitrate(double fps) const;
	static void registerObjects();
	qreal cache() const;
	int droppedFrames() const;
	void setChannelLayoutMap(const ChannelLayoutMap &map);
	void setChannelLayout(ChannelLayout layout);
	ChapterInfoObject *chapterInfo() const;
	AudioTrackInfoObject *audioTrackInfo() const;
	SubtitleTrackInfoObject *subtitleTrackInfo() const;
	void setSubtitleTracks(const QStringList &tracks);
	void setCurrentSubtitleIndex(int idx);
	void sendMouseClick(const QPointF &pos);
	void sendMouseMove(const QPointF &pos);
	void sendDVDCommand(DVDCmd cmd);
	QList<SubtitleFileInfo> subtitleFiles() const;
	void setSubtitleDelay(int ms);
	void setNextStartInfo(const StartInfo &startInfo);
	void shutdown();
public slots:
	void setVolume(int volume);
	void setAmp(double amp);
	void setMuted(bool muted);
	void setVideoRenderer(VideoRendererItem *renderer);
//	void play();
	void stop();
//	void reload();
	void pause();
	void unpause();
	void seek(int pos);
	void relativeSeek(int pos);
signals:
	void sought();
	void tempoScaledChanged(bool on);
	void volumeNormalizerActivatedChanged(bool on);
	void started(Mrl mrl);
	void finished(Mrl mrl, int position, int remain);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(PlayEngine::State state);
	void seekableChanged(bool seekable);
	void durationChanged(int duration);
	void beginChanged(int begin);
	void endChanged(int end);
	void volumeChanged(int volume);
	void preampChanged(double amp);
	void mutedChanged(bool muted);
	void videoFormatChanged(const VideoFormat &format);
	void audioStreamsChanged(const StreamList &streams);
	void videoStreamsChanged(const StreamList &streams);
	void subtitleStreamsChanged(const StreamList &streams);
	void chaptersChanged(const ChapterList &chapters);
	void dvdInfoChanged();
	void speedChanged(double speed);
	void audioChanged();
	void videoChanged();
	void runningChanged();
	void relativePositionChanged();
	void hwaccChanged();
	void cacheChanged();
	void hasVideoChanged();
	void droppedFramesChanged();
	void currentChapterChanged(int chapter);
	void currentAudioStreamChanged(int stream);
	void currentSubtitleStreamChanged(int stream);
	void currentVideoStreamChanged(int stream);
	void subtitleTrackInfoChanged();
	void requestNextStartInfo();
private:
	void updateState(State state);
	void exec();
	void setState(PlayEngine::State state);
	void customEvent(QEvent *event);
	class Thread; struct Data; Data *d;
	PlayEngine::State m_state = PlayEngine::Stopped;
};

#endif // PLAYENGINE_HPP
