#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include "mrl.hpp"
#include "mediamisc.hpp"
#include "enum/videoeffect.hpp"
#include <QQmlListProperty>

class VideoRenderer;                    class HistoryModel;
struct DeintOptionSet;                  class ChannelLayoutMap;
class AudioFormat;                      class VideoColor;
class MetaData;                         struct OsdStyle;
struct AudioNormalizerOption;           class QQuickItem;
enum class ClippingMethod;              enum class VideoEffect;
enum class DeintMethod;                 enum class DeintMode;
enum class ChannelLayout;               enum class Interpolator;
enum class ColorRange;                  enum class ColorSpace;
enum class Dithering;                   enum class AutoselectMode;
enum class VideoRatio;                  enum class SubtitleDisplay;
enum class VerticalAlignment;           enum class HorizontalAlignment;
enum class CodecId;
class AudioObject;                      class VideoObject;
class YouTubeDL;                        struct AudioDevice;
class YleDL;                            class AudioEqualizer;
class StreamTrack;                      class SubtitleObject;
class OpenGLFramebufferObject;          class SubtitleRenderer;
class SubCompModel;                     class MrlState;
class QOpenGLContext;                   class EncodingInfo;
class SubComp;
struct Autoloader;                      struct CacheInfo;
struct IntrplParamSet;                  struct MotionIntrplOption;

struct StringPair { QString s1, s2; };
using IntrplParamSetMap = QMap<Interpolator, IntrplParamSet>;

class PlayEngine : public QObject {
    Q_OBJECT
    Q_ENUMS(State)
    Q_ENUMS(ActivationState)
    Q_ENUMS(Waiting)

    Q_PROPERTY(MediaObject *media READ media CONSTANT FINAL)
    Q_PROPERTY(AudioObject *audio READ audio CONSTANT FINAL)
    Q_PROPERTY(VideoObject *video READ video CONSTANT FINAL)
    Q_PROPERTY(SubtitleObject* subtitle READ subtitle CONSTANT FINAL)

    Q_PROPERTY(int begin READ begin NOTIFY beginChanged)
    Q_PROPERTY(int end READ end NOTIFY endChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int time READ time WRITE seek NOTIFY tick)
    Q_PROPERTY(qreal rate READ rate WRITE setRate NOTIFY tick)

    Q_PROPERTY(int begin_s READ begin_s NOTIFY begin_sChanged)
    Q_PROPERTY(int end_s READ end_s NOTIFY end_sChanged)
    Q_PROPERTY(int duration_s READ duration_s NOTIFY duration_sChanged)
    Q_PROPERTY(int time_s READ time_s NOTIFY time_sChanged)

    Q_PROPERTY(qreal volume READ volume WRITE setAudioVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)

    Q_PROPERTY(int cacheSize READ cacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(int cacheUsed READ cacheUsed NOTIFY cacheUsedChanged)
    Q_PROPERTY(int avSync READ avSync NOTIFY avSyncChanged)

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool stopped READ isStopped NOTIFY stoppedChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(Waiting waiting READ waiting NOTIFY waitingChanged)
    Q_PROPERTY(QString waitingText READ waitingText NOTIFY waitingChanged)

    Q_PROPERTY(qreal speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(EditionChapterObject* chapter READ chapter NOTIFY chapterChanged)
    Q_PROPERTY(EditionChapterObject* edition READ edition NOTIFY editionChanged)
    Q_PROPERTY(QQmlListProperty<EditionChapterObject> chapters READ chapterList NOTIFY chaptersChanged)
    Q_PROPERTY(QQmlListProperty<EditionChapterObject> editions READ editionList NOTIFY editionsChanged)
public:
    enum State { Stopped = 1, Playing = 2, Paused = 4, Error = 32, Running = Playing | Paused };
    enum Waiting { NoWaiting = 0, Searching = 1, Loading = 2, Buffering = 4, Seeking = 8 };
    Q_DECLARE_FLAGS(Waitings, Waiting)
    enum ActivationState { Unavailable, Deactivated, Activated };
    enum Snapshot {
        NoSnapshot = 0, VideoOnly = 1, VideoWidthOsd = 2,
        VideoAndOsd = VideoOnly | VideoWidthOsd
    };
    enum DVDCmd { DVDMenu = -1 };
    PlayEngine();
    ~PlayEngine();

    auto restore(const MrlState *params) -> void;

    auto isWaiting() const -> bool;
    auto waiting() const -> Waiting;
    auto time() const -> int;
    auto begin() const -> int;
    auto end() const -> int;
    auto duration() const -> int;
    auto mrl() const -> Mrl;
    auto isSeekable() const -> bool;
    auto isPlaying() const -> bool {return state() & Playing;}
    auto isPaused() const -> bool {return state() & Paused;}
    auto isStopped() const -> bool {return state() & Stopped;}
    auto isRunning() const -> bool { return state() & Running; }
    auto speed() const -> double;
    auto state() const -> State;
    auto load(const Mrl &mrl, bool tryResume = true) -> void;
    auto setMrl(const Mrl &mrl) -> void;
    auto edition() const -> EditionObject*;
    auto chapter() const -> ChapterObject*;
    auto editions() const -> const QVector<EditionObject*>&;
    auto chapters() const -> const QVector<ChapterObject*>&;
    auto seekEdition(int number, int from = 0) -> void;
    auto seekChapter(int number) -> void;

    auto setAudioFiles(const QStringList &files) -> void;
    auto addAudioFiles(const QStringList &files) -> void;
    auto clearAudioFiles() -> void;

    auto subtitleSelection() const -> QVector<SubComp>;
    auto setSubtitleDisplay(SubtitleDisplay sd) -> void;
    auto setSubtitlePosition(double pos) -> void;
    auto setSubtitleAlignment(VerticalAlignment a) -> void;
    auto setSubtitleFiles(const QStringList &files, const EncodingInfo &enc) -> void;
    auto addSubtitleFiles(const QStringList &files, const EncodingInfo &enc) -> void;
    auto clearSubtitleFiles() -> void;
    auto captionBeginTime() -> int;
    auto captionEndTime() -> int;
    auto seekCaption(int direction) -> void;
    auto subtitleImage(const QRect &rect, QRectF *subRect = nullptr) const -> QImage;

    auto setSubtitleTrackSelected(int id, bool s) -> void;
    auto setVideoTrackSelected(int id, bool s) -> void;
    auto setAudioTrackSelected(int id, bool s) -> void;
    auto setSubtitleInclusiveTrackSelected(int id, bool s) -> void;

    auto lock() -> void;
    auto setHwAcc_locked(bool use, const QList<CodecId> &codecs) -> void;
    auto setSubtitleStyle_locked(const OsdStyle &style) -> void;
    auto setSubtitleEncoding_locked(const EncodingInfo &enc, double accuracy) -> void;
    auto setAutoselectMode_locked(bool enable, AutoselectMode mode, const QString &ext) -> void;
    auto setCache_locked(const CacheInfo &info) -> void;
    auto setVolumeNormalizerOption_locked(const AudioNormalizerOption &option) -> void;
    auto setDeintOptions_locked(const DeintOptionSet &set) -> void;
    auto setAudioDevice_locked(const QString &device) -> void;
    auto setClippingMethod_locked(ClippingMethod method) -> void;
    auto setChannelLayoutMap_locked(const ChannelLayoutMap &map) -> void;
    auto setPriority_locked(const QStringList &audio, const QStringList &sub) -> void;
    auto setAutoloader_locked(const Autoloader &audio, const Autoloader &sub) -> void;
    auto setResume_locked(bool resume) -> void;
    auto setPreciseSeeking_locked(bool on) -> void;
    auto setMotionIntrplOption_locked(const MotionIntrplOption &option) -> void;
    auto unlock() -> void;

    auto params() const -> const MrlState*;
    auto default_() const -> const MrlState*;
    auto videoOutputAspectRatio() const -> double;
    auto adjustVideoAspectRatio(double by) -> void;
    auto setVideoAspectRatio(double ratio) -> void;
    auto setVideoCropRatio(double ratio) -> void;
    auto setVideoHighQualityUpscaling(bool on) -> void;
    auto setVideoHighQualityDownscaling(bool on) -> void;
    auto setVideoVerticalAlignment(VerticalAlignment a) -> void;
    auto setVideoHorizontalAlignment(HorizontalAlignment a) -> void;
    auto hasVideoFrame() const -> bool;
    auto setVideoOffset(const QPointF &offset) -> void;
    auto videoSizeHint() const -> QSize;
    auto hasVideo() const -> bool;
    auto setAudioVolumeNormalizer(bool on) -> void;
    auto setAudioTempoScaler(bool on) -> void;
    auto setSubtitleVisible(bool visible) -> void { setSubtitleHidden(!visible); }
    auto setSubtitleHidden(bool hidden) -> void;
    auto autoloadSubtitleFiles() -> void;
    auto autoloadAudioFiles() -> void;
    auto reloadSubtitleFiles(const EncodingInfo &enc, double acc = -1) -> void;
    auto reloadAudioFiles() -> void;
    auto setSubtitleStyleOverriden(bool override) -> void;

    auto setSpeed(double speed) -> void;
    auto setAudioSync(int sync) -> void;
    auto audioSync() const -> int;
    auto metaData() const -> const MetaData&;
    auto volume() const -> double;
    auto isMuted() const -> bool;

    auto setDeintMode(DeintMode mode) -> void;
    auto deintMode() const -> DeintMode;
    auto run() -> void;
    auto waitUntilTerminated() -> void;
    auto screen() const -> QQuickItem*;
    auto media() const -> MediaObject*;
    auto audio() const -> AudioObject*;
    auto video() const -> VideoObject*;
    auto avSync() const -> int;
    auto rate(int time) const -> double { return (double)(time-begin())/duration(); }
    auto rate() const -> double { return rate(time()); }
    auto setRate(qreal r) -> void { seek(begin() + r * duration()); }
    auto cacheSize() const -> int;
    auto cacheUsed() const -> int;
    auto setChannelLayout(ChannelLayout layout) -> void;
    auto chapterList() const -> QQmlListProperty<EditionChapterObject>;
    auto editionList() const -> QQmlListProperty<EditionChapterObject>;
    auto setYle(YleDL *yle) -> void;
    auto setYouTube(YouTubeDL *yt) -> void;
    auto sendMouseClick(const QPointF &pos) -> void;
    auto sendMouseMove(const QPointF &pos) -> void;
    auto subtitle() const -> SubtitleObject*;
    auto setSubtitleDelay(int ms) -> void;
    auto setNextMrl(const Mrl &Mrl) -> void;
    auto shutdown() -> void;
    auto stepFrame(int direction) -> void;
    auto setAudioVolume(double volume) -> void;
    auto setAudioAmp(double amp) -> void;
    auto setAudioMuted(bool muted) -> void;
    auto setAudioEqualizer(const AudioEqualizer &eq) -> void;
    auto audioDeviceList() const -> QList<AudioDevice>;
    auto stop() -> void;
    auto reload() -> void;
    auto pause() -> void;
    auto unpause() -> void;
    auto relativeSeek(int pos) -> void;
    auto seekToNextBlackFrame() -> void;

    auto initializeGL(QOpenGLContext *ctx) -> void;
    auto finalizeGL(QOpenGLContext *ctx) -> void;

    auto setColorRange(ColorRange range) -> void;
    auto setColorSpace(ColorSpace space) -> void;
    auto setVideoEqualizer(const VideoColor &eq) -> void;
    auto setInterpolator(Interpolator type) -> void;
    auto setChromaUpscaler(Interpolator type) -> void;
    auto setInterpolator(const IntrplParamSet &params) -> void;
    auto setChromaUpscaler(const IntrplParamSet &params) -> void;
    auto setInterpolatorMap(const IntrplParamSetMap &map) -> void;
    auto setChromaUpscalerMap(const IntrplParamSetMap &map) -> void;
    auto setMotionInterpolation(bool on) -> void;
    auto interpolator() const -> IntrplParamSet;
    auto chromaUpscaler() const -> IntrplParamSet;
    auto interpolatorMap() const -> IntrplParamSetMap;
    auto chromaUpscalerMap() const -> IntrplParamSetMap;
    auto setVideoDithering(Dithering dithering) -> void;
    auto setVideoEffects(VideoEffects effects) -> void;
    auto takeSnapshot(Snapshot mode) -> void;
    auto snapshot(bool withOsd = true) -> QImage;
    auto clearSnapshots() -> void;
    auto setHighQualityScaling(bool up, bool down) -> void;
    auto waitingText() const -> QString;
    auto stateText() const -> QString;

    auto time_s() const -> int;
    auto duration_s() const -> int;
    auto begin_s() const -> int;
    auto end_s() const -> int;
    auto setHistory(HistoryModel *history) -> void;
public slots:
    void seek(int pos);
signals:
    void time_sChanged();
    void duration_sChanged();
    void begin_sChanged();
    void end_sChanged();

    void beginSyncMrlState();
    void endSyncMrlState();
    void seeked(int time);
    void sought();
    void started(Mrl mrl);
    void finished(Mrl mrl, bool eof);
    void tick(int pos);
    void mrlChanged(const Mrl &mrl);
    void stateChanged(PlayEngine::State state);
    void seekableChanged(bool seekable);
    void durationChanged(int duration);
    void beginChanged(int begin);
    void endChanged();
    void volumeChanged();
    void mutedChanged();
    void avSyncChanged(int avSync);
    void chaptersChanged();
    void editionsChanged();
    void editionChanged();
    void dvdInfoChanged();
    void speedChanged();
    void hwaccChanged();
    void cacheUsedChanged();
    void hasVideoChanged();
    void chapterChanged();
    void subtitleTrackInfoChanged();
    void metaDataChanged();
    void waitingChanged(Waiting waiting);
    void pausedChanged();
    void playingChanged();
    void stoppedChanged();
    void runningChanged();
    void deintOptionsChanged();
    void cacheSizeChanged();
    void messageRequested(const QString &message);
    void snapshotTaken();
    void subtitleSelectionChanged();
private:
    auto customEvent(QEvent *event) -> void;
    struct Data; Data *d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PlayEngine::Waitings)

#endif // PLAYENGINE_HPP
