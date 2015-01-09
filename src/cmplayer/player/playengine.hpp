#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "mrl.hpp"
#include "mediamisc.hpp"
#include "enum/videoeffect.hpp"

class VideoRenderer;                    class VideoFormat;
class DeintOption;                      class ChannelLayoutMap;
class AudioFormat;                      class VideoColor;
class MetaData;                         struct OsdTheme;
struct AudioNormalizerOption;           struct SubtitleFileInfo;
enum class ClippingMethod;              enum class VideoEffect;
enum class DeintMethod;                 enum class DeintMode;
enum class ChannelLayout;               enum class Interpolator;
enum class ColorRange;                  enum class ColorSpace;
enum class Dithering;
class AudioInfoObject;                  class VideoInfoObject;
class YouTubeDL;                    struct AudioDevice;
class StreamTrack;                      class SubtitleInfoObject;
class OpenGLFramebufferObject;
using StreamList = QMap<int, StreamTrack>;

struct StartInfo {
    StartInfo() {}
    StartInfo(const Mrl &mrl): mrl(mrl) {}
    Mrl mrl;
    int resume = -1, cache = -1, edition = -1;
    auto isValid() const -> bool
    { return (!mrl.isEmpty() || mrl.isDisc()) && resume >= 0 && cache >= 0; }
};

struct FinishInfo {
    Mrl mrl;
    int position = 0, remain = 0;
    QVector<int> streamIds = { 0, 0, 0 };
};


class PlayEngine : public QObject {
    Q_OBJECT
    Q_ENUMS(State)
    Q_ENUMS(ActivationState)
    Q_PROPERTY(MediaInfoObject *media READ mediaInfo CONSTANT FINAL)
    Q_PROPERTY(AudioInfoObject *audio READ audioInfo CONSTANT FINAL)
    Q_PROPERTY(VideoInfoObject *video READ videoInfo CONSTANT FINAL)
    Q_PROPERTY(int begin READ begin NOTIFY beginChanged)
    Q_PROPERTY(int end READ end NOTIFY endChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int time READ time NOTIFY tick)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(int cacheUsed READ cacheUsed NOTIFY cacheUsedChanged)
    Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
    Q_PROPERTY(double volumeNormalizer READ volumeNormalizer)
    Q_PROPERTY(int avSync READ avSync NOTIFY avSyncChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(double speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(bool volumeNormalizerActivated READ isVolumeNormalizerActivated NOTIFY volumeNormalizerActivatedChanged)
    Q_PROPERTY(double rate READ rate NOTIFY tick)
    Q_PROPERTY(QQuickItem *screen READ screen)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(ChapterInfoObject *chapter READ chapterInfo NOTIFY chaptersChanged)
    Q_PROPERTY(SubtitleInfoObject* subtitle READ subInfo NOTIFY subInfoChanged)
public:
    enum State {
        Stopped = 1, Playing = 2, Paused = 4,
        Loading = 16, Error = 32, Buffering = 64
//        Running = Playing | Loading | Buffering
    };
    static constexpr int Running = Playing | Loading | Buffering;
    enum ActivationState { Unavailable, Deactivated, Activated };
    enum Snapshot {
        NoSnapshot = 0, VideoOnly = 1, VideoWidthOsd = 2,
        VideoAndOsd = VideoOnly | VideoWidthOsd
    };
    enum DVDCmd { DVDMenu = -1 };
    PlayEngine();
    ~PlayEngine();

    auto time() const -> int;
    auto begin() const -> int;
    auto end() const -> int;
    auto duration() const -> int;
    auto mrl() const -> Mrl;
    auto isSeekable() const -> bool;
    auto setHwAcc(bool use, const QStringList &codecs) -> void;
    auto isRunning() const -> bool { return m_state & Running; }
    auto isPlaying() const -> bool {return m_state & Playing;}
    auto isPaused() const -> bool {return m_state & Paused;}
    auto isStopped() const -> bool {return m_state & Stopped;}
    auto speed() const -> double;
    auto state() const -> State { return m_state; }
    auto load(const StartInfo &info) -> void;
    auto startInfo() const -> const StartInfo&;
    auto setSpeed(double speed) -> void;
    auto currentEdition() const -> int;
    auto editions() const -> const EditionList&;
    auto currentChapter() const -> int;
    auto chapters() const -> const ChapterList&;
    auto currentSubtitleStream() const -> int;
    auto subtitleStreams() const -> const StreamList&;
    auto setCurrentSubtitleStream(int id, bool reserve = false) -> void;
    auto setCurrentEdition(int id, int from = 0) -> void;
    auto setCurrentChapter(int id) -> void;
    auto setSubtitleStyle(const OsdTheme &style) -> void;
    auto hasVideo() const -> bool;
    auto setVolumeNormalizerActivated(bool on) -> void;
    auto setTempoScalerActivated(bool on) -> void;
    auto isVolumeNormalizerActivated() const -> bool;
    auto isTempoScaled() const -> bool;
    auto videoRenderer() const -> VideoRenderer*;
    auto videoFormat() const -> VideoFormat;
    auto videoStreams() const -> const StreamList&;
    auto setCurrentVideoStream(int id) -> void;
    auto currentVideoStream() const -> int;
    auto setAudioSync(int sync) -> void;
    auto audioSync() const -> int;
    auto metaData() const -> const MetaData&;
    auto mediaName() const -> QString;
    auto volume() const -> int;
    auto currentAudioStream() const -> int;
    auto isMuted() const -> bool;
    auto volumeNormalizer() const -> double;
    auto amp() const -> double;
    auto audioStreams() const -> const StreamList&;
    auto setCurrentAudioStream(int id, bool reserve = false) -> void;
    auto setVolumeNormalizerOption(const AudioNormalizerOption &option) -> void;
    auto addSubtitleStream(const QString &fileName, const QString &enc) -> bool;
    auto removeSubtitleStream(int id) -> void;
    auto setSubtitleStreamsVisible(bool visible) -> void;
    auto isSubtitleStreamsVisible() const -> bool;
    auto setDeintOptions(const DeintOption &swdec,
                         const DeintOption &hwdec) -> void;
    auto deintOptionForSwDec() const -> DeintOption;
    auto deintOptionForHwDec() const -> DeintOption;
    auto setDeintMode(DeintMode mode) -> void;
    auto deintMode() const -> DeintMode;
    auto setAudioDevice(const QString &device) -> void;
    auto setClippingMethod(ClippingMethod method) -> void;
    auto setMinimumCache(qreal playback, qreal seeking) -> void;
    auto run() -> void;
    auto waitUntilTerminated() -> void;
    auto thread() const -> QThread*;
    auto screen() const -> QQuickItem*;
    auto mediaInfo() const -> MediaInfoObject*;
    auto audioInfo() const -> AudioInfoObject*;
    auto videoInfo() const -> VideoInfoObject*;
    auto avSync() const -> int;
    auto stateText() const -> QString { return stateText(m_state); }
    auto rate() const -> double { return (double)(time()-begin())/duration(); }
    auto cacheSize() const -> int;
    auto cacheUsed() const -> int;
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void;
    auto setChannelLayout(ChannelLayout layout) -> void;
    auto chapterInfo() const -> ChapterInfoObject*;
    auto setSubtitleFiles(const StreamList &files) -> void;
    auto setYouTube(YouTubeDL *yt) -> void;
    auto sendMouseClick(const QPointF &pos) -> void;
    auto sendMouseMove(const QPointF &pos) -> void;
    auto mousePosition() const -> const QPoint& { return m_mouse; }
    auto subInfo() const -> SubtitleInfoObject*;
    auto subtitleFiles() const -> QVector<SubtitleFileInfo>;
    auto setSubtitleDelay(int ms) -> void;
    auto setNextStartInfo(const StartInfo &startInfo) -> void;
    auto shutdown() -> void;
    auto stepFrame(int direction) -> void;
    auto setVolume(int volume) -> void;
    auto setAmp(double amp) -> void;
    auto setMuted(bool muted) -> void;
    auto audioDeviceList() const -> QList<AudioDevice>;
    auto stop() -> void;
    auto reload() -> void;
    auto pause() -> void;
    auto unpause() -> void;
    auto relativeSeek(int pos) -> void;
    auto isOffscreenInitialized() const -> bool { return m_offscreenInit; }
    auto setAudioPriority(const QStringList &ap) -> void;
    auto setSubtitlePriority(const QStringList &sp) -> void;
    auto initializeGL(QOpenGLContext *ctx) -> void;
    auto finalizeGL(QOpenGLContext *ctx) -> void;

    auto setColorRange(ColorRange range) -> void;
    auto setColorSpace(ColorSpace space) -> void;
    auto colorRange() const -> ColorRange;
    auto colorSpace() const -> ColorSpace;
    auto videoEqualizer() const -> VideoColor;
    auto setVideoEqualizer(const VideoColor &eq) -> void;
    auto setInterpolator(Interpolator type) -> void;
    auto setChromaUpscaler(Interpolator type) -> void;
    auto interpolator() const -> Interpolator;
    auto chromaUpscaler() const -> Interpolator;
    auto setDithering(Dithering dithering) -> void;
    auto dithering() const -> Dithering;
    auto setVideoEffects(VideoEffects effects) -> void;
    auto videoEffects() const -> VideoEffects;
    auto takeSnapshot(Snapshot mode) -> void;
    Q_INVOKABLE double bitrate(double fps) const;
    Q_INVOKABLE void seek(int pos);
    static auto stateText(State state) -> QString;
signals:
    void subInfoChanged();
    void seeked(int time);
    void sought();
    void tempoScaledChanged(bool on);
    void volumeNormalizerActivatedChanged(bool on);
    void started(Mrl mrl);
    void finished(const FinishInfo &info);
    void tick(int pos);
    void mrlChanged(const Mrl &mrl);
    void stateChanged(PlayEngine::State state);
    void seekableChanged(bool seekable);
    void durationChanged(int duration);
    void beginChanged(int begin);
    void endChanged();
    void volumeChanged(int volume);
    void preampChanged(double amp);
    void mutedChanged(bool muted);
    void avSyncChanged(int avSync);
    void videoFormatChanged(const VideoFormat &format);
    void audioStreamsChanged(const StreamList &streams);
    void videoStreamsChanged(const StreamList &streams);
    void subtitleStreamsChanged(const StreamList &streams);
    void chaptersChanged(const ChapterList &chapters);
    void editionsChanged(const EditionList &editions);
    void dvdInfoChanged();
    void speedChanged(double speed);
    void runningChanged();
    void hwaccChanged();
    void cacheUsedChanged();
    void hasVideoChanged();
    void currentChapterChanged(int chapter);
    void currentAudioStreamChanged(int stream);
    void currentSubtitleStreamChanged(int stream);
    void currentVideoStreamChanged(int stream);
    void subtitleTrackInfoChanged();
    void requestNextStartInfo();
    void metaDataChanged();

    void deintOptionsChanged();
    void cacheSizeChanged();
    void messageRequested(const QString &message);
    void snapshotTaken(QImage video, QImage osd);
private:
    auto updateState(State state) -> void;
    auto exec() -> void;
    auto customEvent(QEvent *event) -> void;
    auto updateVideoFormat(VideoFormat format) -> void;
    class Thread; struct Data; Data *d;
    template<class T>
    friend class SimpleObservation;
    PlayEngine::State m_state = PlayEngine::Stopped;
    QPoint m_mouse;
    bool m_offscreenInit = false;
};

#endif // PLAYENGINE_HPP
