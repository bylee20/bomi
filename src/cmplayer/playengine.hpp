#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "global.hpp"
#include "mediamisc.hpp"
#include <functional>

class VideoRendererItem;                class VideoFormat;
class DeintOption;                      class ChannelLayoutMap;
class AudioFormat;
class MetaData;                         class SubtitleStyle;
struct AudioNormalizerOption;           struct SubtitleFileInfo;
enum class AudioDriver;                 enum class ClippingMethod;
enum class DeintMethod;                 enum class DeintMode;
enum class ChannelLayout;               class VideoColor;
enum class ColorRange;                  enum class InterpolatorType;

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
    Q_PROPERTY(double rate READ rate NOTIFY rateChanged)
    Q_PROPERTY(QQuickItem *screen READ screen)
    Q_PROPERTY(qreal cache READ cache NOTIFY cacheChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(ChapterInfoObject *chapter READ chapterInfo NOTIFY chaptersChanged)
    Q_PROPERTY(AudioTrackInfoObject *audioTrack READ audioTrackInfo NOTIFY audioStreamsChanged)
    Q_PROPERTY(SubtitleTrackInfoObject *subtitleTrack READ subtitleTrackInfo NOTIFY subtitleTrackInfoChanged)
public:
    enum State {
        Stopped = 1, Playing = 2, Paused = 4,
        Loading = 16, Error = 32, Buffering = 64,
        Running = Playing | Loading | Buffering
    };
    enum class HardwareAcceleration { Unavailable, Deactivated, Activated };
    enum DVDCmd { DVDMenu = -1 };
    PlayEngine();
    ~PlayEngine();
    auto initializeGL(QQuickWindow *window) -> void;
    auto finalizeGL() -> void;
    auto videoEqualizer() const -> const VideoColor&;
    auto setVideoEqualizer(const VideoColor &prop) -> void;
    auto setVideoColorRange(ColorRange range) -> void;
    auto videoColorRange() const -> ColorRange;
    auto setVideoChromaUpscaler(InterpolatorType tpe) -> void;
    auto videoChromaUpscaler() const -> InterpolatorType;

    auto time() const -> int;
    auto begin() const -> int;
    auto end() const -> int;
    auto setImageDuration(int duration) -> void;
    auto duration() const -> int;
    auto mrl() const -> Mrl;
    auto isSeekable() const -> bool;
    auto setHwAcc(int backend, const QList<int> &codecs) -> void;
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
    auto setCurrentSubtitleStream(int id) -> void;
    auto setCurrentEdition(int id, int from = 0) -> void;
    auto setCurrentChapter(int id) -> void;
    auto setSubtitleStyle(const SubtitleStyle &style) -> void;
    auto hasVideo() const -> bool;
    auto setVolumeNormalizerActivated(bool on) -> void;
    auto setTempoScalerActivated(bool on) -> void;
    auto isVolumeNormalizerActivated() const -> bool;
    auto isTempoScaled() const -> bool;
    auto fps() const -> double;
    auto videoRenderer() const -> VideoRendererItem*;
    auto videoFormat() const -> VideoFormat;
    auto videoStreams() const -> const StreamList&;
    auto setCurrentVideoStream(int id) -> void;
    auto currentVideoStream() const -> int;
    auto setAudioSync(int sync) -> void;
    auto audioSync() const -> int;
    auto metaData() const -> const MetaData&;
    auto mediaName() const -> QString;
    auto hwAcc() const -> HardwareAcceleration;
    auto volume() const -> int;
    auto currentAudioStream() const -> int;
    auto isMuted() const -> bool;
    auto volumeNormalizer() const -> double;
    auto amp() const -> double;
    auto audioStreams() const -> const StreamList&;
    auto setCurrentAudioStream(int id) -> void;
    auto setVolumeNormalizerOption(const AudioNormalizerOption &option) -> void;
    auto addSubtitleStream(const QString &fileName, const QString &enc) -> bool;
    auto removeSubtitleStream(int id) -> void;
    auto setSubtitleStreamsVisible(bool visible) -> void;
    auto isSubtitleStreamsVisible() const -> bool;
    auto setDeintOptions(const DeintOption &swdec,
                         const DeintOption &hwdec) -> void;
    auto setDeintMode(DeintMode mode) -> void;
    auto deintMode() const -> DeintMode;
    auto setAudioDriver(AudioDriver driver) -> void;
    auto setClippingMethod(ClippingMethod method) -> void;
    auto setMinimumCache(int playback, int seeking) -> void;
    auto run() -> void;
    auto waitUntilTerminated() -> void;
    auto thread() const -> QThread*;
    auto screen() const -> QQuickItem*;
    auto mediaInfo() const -> MediaInfoObject*;
    auto audioInfo() const -> AvInfoObject*;
    auto videoInfo() const -> AvInfoObject*;
    auto avgsync() const -> double;
    auto avgfps() const -> double;
    auto stateText() const -> QString { return stateText(m_state); }
    auto rate() const -> double { return (double)(time()-begin())/duration(); }
    auto cache() const -> qreal;
    auto droppedFrames() const -> int;
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void;
    auto setChannelLayout(ChannelLayout layout) -> void;
    auto chapterInfo() const -> ChapterInfoObject*;
    auto audioTrackInfo() const -> AudioTrackInfoObject*;
    auto subtitleTrackInfo() const -> SubtitleTrackInfoObject*;
    auto setSubtitleTracks(const QStringList &tracks) -> void;
    auto setCurrentSubtitleIndex(int idx) -> void;
    auto sendMouseClick(const QPointF &pos) -> void;
    auto sendMouseMove(const QPointF &pos) -> void;
    auto mousePosition() const -> const QPoint& { return m_mouse; }
    auto subtitleFiles() const -> QList<SubtitleFileInfo>;
    auto setSubtitleDelay(int ms) -> void;
    auto setNextStartInfo(const StartInfo &startInfo) -> void;
    auto shutdown() -> void;
    auto stepFrame(int direction) -> void;
    auto setVolume(int volume) -> void;
    auto setAmp(double amp) -> void;
    auto setMuted(bool muted) -> void;
    auto setVideoRenderer(VideoRendererItem *renderer) -> void;
    auto stop() -> void;
    auto reload() -> void;
    auto pause() -> void;
    auto unpause() -> void;
    auto relativeSeek(int pos) -> void;
    Q_INVOKABLE double bitrate(double fps) const;
    Q_INVOKABLE void seek(int pos);
    static auto stateText(State state) -> QString;
    static auto registerObjects() -> void;
signals:
    void fpsChanged(double fps);
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
    void endChanged(int end);
    void volumeChanged(int volume);
    void preampChanged(double amp);
    void mutedChanged(bool muted);
    void videoFormatChanged(const VideoFormat &format);
    void audioStreamsChanged(const StreamList &streams);
    void videoStreamsChanged(const StreamList &streams);
    void subtitleStreamsChanged(const StreamList &streams);
    void chaptersChanged(const ChapterList &chapters);
    void editionsChanged(const EditionList &editions);
    void dvdInfoChanged();
    void speedChanged(double speed);
    void audioChanged();
    void videoChanged();
    void runningChanged();
    void rateChanged();
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
    void metaDataChanged();
    void videoColorRangeChanged(ColorRange range);
    void videoChromaUpscalerChanged(InterpolatorType type);
private:
    auto updateState(State state) -> void;
    auto exec() -> void;
    auto setState(PlayEngine::State state) -> void;
    auto customEvent(QEvent *event) -> void;
    auto updateVideoFormat(VideoFormat format) -> void;
    class Thread; struct Data; Data *d;
    PlayEngine::State m_state = PlayEngine::Stopped;
    QPoint m_mouse;
};

#endif // PLAYENGINE_HPP
