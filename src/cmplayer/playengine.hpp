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
class DeintOption;
enum class AudioDriver;		enum class ClippingMethod;
enum class DeintMethod;		enum class DeintMode;

typedef std::function<int(const Mrl&)> GetMrlInt;

typedef QLinkedList<QString> FilterList;

class PlayEngine : public QObject {
	Q_OBJECT
	Q_ENUMS(State)
	Q_ENUMS(HardwareAcceleration)
	Q_PROPERTY(MediaInfoObject *media READ mediaInfo NOTIFY mediaChanged)
	Q_PROPERTY(AvInfoObject *audio READ audioInfo NOTIFY videoChanged)
	Q_PROPERTY(AvInfoObject *video READ videoInfo NOTIFY audioChanged)
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
	Q_PROPERTY(HardwareAcceleration hardwareAccelaration READ hwAcc NOTIFY hwAccChanged)
	Q_PROPERTY(QString hardwareAccelerationText READ hwAccText NOTIFY hwAccChanged)
	Q_PROPERTY(double relativePosition READ relativePosition NOTIFY relativePositionChanged)
	Q_PROPERTY(QQuickItem *screen READ screen)
	Q_PROPERTY(qreal cache READ cache NOTIFY cacheChanged)
	Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
	Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
public:
	enum State {Stopped = 1, Playing = 2, Paused = 4, Finished = 8, Loading = 16, Error = 32, Running = Playing | Loading };
	enum class HardwareAcceleration { Unavailable, Deactivated, Activated };
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
	void setPlaylist(const Playlist &playlist);
	Mrl mrl() const;
	bool atEnd() const;
	bool isSeekable() const;
	void setHwAccCodecs(const QList<int> &codecs);
	bool isRunning() const { return m_state & Running; }
	bool isPlaying() const {return m_state & Playing;}
	bool isPaused() const {return m_state & Paused;}
	bool isStopped() const {return m_state & Stopped;}
	bool isFinished() const {return m_state & Finished;}
	bool isInitialized() const;
	double speed() const;
	PlayEngine::State state() const { return m_state; }
	void load(const Mrl &mrl, int start = -1);
	void load(const Mrl &mrl, bool play);
	void setSpeed(double speed);
	const DvdInfo &dvd() const;
	int currentDvdTitle() const;
	int currentChapter() const;
	ChapterList chapters() const;
	int currentSubtitleStream() const;
	StreamList subtitleStreams() const;
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
	StreamList videoStreams() const;
	void setCurrentVideoStream(int id);
	int currentVideoStream() const;
	void setGetStartTimeFunction(const GetMrlInt &func);
	void setGetCacheFunction(const GetMrlInt &func);
	void setAudioSync(int sync);
	int audioSync() const;
	const PlaylistModel &playlist() const;
	PlaylistModel &playlist();

	HardwareAcceleration hwAcc() const;
	QString hwAccText() const;
	int volume() const;
	int currentAudioStream() const;
	bool isMuted() const;
	double volumeNormalizer() const;
	double amp() const;
	StreamList audioStreams() const;
	void setCurrentAudioStream(int id);
	void setVolumeNormalizerOption(double length, double target, double silence, double min, double max);
	void addSubtitleStream(const QString &fileName, const QString &enc);
	void removeSubtitleStream(int id);
	void setSubtitleStreamsVisible(bool visible);
	bool isSubtitleStreamsVisible() const;
	void setVideoFilters(const QString &vfs);
	void setDeintOptions(const DeintOption &swdec, const DeintOption &hwdec);
	void setDeintMode(DeintMode mode);
	DeintMode deintMode() const;
	void setAudioDriver(AudioDriver driver);
	AudioDriver preferredAudioDriver() const;
	AudioDriver audioDriver() const;
	void setClippingMethod(ClippingMethod method);
	void setMinimumCache(int playback, int seeking);
	void run();
	void waitUntilTerminated();
	void waitUntilInitilaized();
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
	Q_INVOKABLE double bps(double fps) const;
	static void registerObjects();
	qreal cache() const;
	int droppedFrames() const;
public slots:
	void setVolume(int volume);
	void setAmp(double amp);
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
	void sought();
	void tempoScaledChanged(bool on);
	void volumeNormalizerActivatedChanged(bool on);
	void started(Mrl mrl);
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
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
	void mediaChanged();
	void audioChanged();
	void videoChanged();
	void runningChanged();
	void relativePositionChanged();
	void hwAccChanged();
	void cacheChanged();
	void hasVideoChanged();
	void droppedFramesChanged();
private:
	int playImage(const Mrl &mrl, int &terminated, int &duration);
	int playAudioVideo(const Mrl &mrl, int &terminated, int &duration);
	void exec();
	void setState(PlayEngine::State state);
	void customEvent(QEvent *event);
	class Thread; struct Data; Data *d;
	PlayEngine::State m_state = PlayEngine::Stopped;
};

#endif // PLAYENGINE_HPP
