#ifndef PLAYENGINE_P_HPP
#define PLAYENGINE_P_HPP

#include "playengine.hpp"
#include "mpv_helper.hpp"
#include "mpv.hpp"
#include "mrlstate_p.hpp"
#include "avinfoobject.hpp"
#include "streamtrack.hpp"
#include "historymodel.hpp"
#include "misc/autoloader.hpp"
#include "misc/youtubedl.hpp"
#include "misc/osdstyle.hpp"
#include "misc/speedmeasure.hpp"
#include "misc/yledl.hpp"
#include "misc/charsetdetector.hpp"
#include "audio/audiocontroller.hpp"
#include "audio/audioformat.hpp"
#include "video/videorenderer.hpp"
#include "video/videoprocessor.hpp"
#include "subtitle/subtitle.hpp"
#include "subtitle/subtitlerenderer.hpp"
#include "enum/codecid.hpp"
#include "opengl/openglframebufferobject.hpp"
#include "os/os.hpp"

#ifdef bool
#undef bool
#endif

DECLARE_LOG_CONTEXT(Engine)

enum EventType {
    UserType = QEvent::User, StateChange, WaitingChange,
    PreparePlayback,EndPlayback, StartPlayback, NotifySeek,
    SyncMrlState,
    EventTypeMax
};

static const QVector<StreamType> streamTypes
    = { StreamAudio, StreamVideo, StreamSubtitle };

struct StreamData {
    StreamData() = default;
    StreamData(const char *pid, ExtType ext)
        : pid(pid), ext(ext) { }
    const char *pid = nullptr;
    ExtType ext;
    int reserved = -1;
    QStringList priority;
    Autoloader autoloader;
};

struct SubtitleWithEncoding {
    QString file;
    EncodingInfo encoding;
};

struct PlayEngine::Data {
    Data(PlayEngine *engine);
    PlayEngine *p = nullptr;

    Mpv mpv;
    VideoRenderer *vr = nullptr;
    AudioController *ac = nullptr;
    SubtitleRenderer *sr = nullptr;
    VideoProcessor *vp = nullptr;

    PlayEngine::Waitings waitings = PlayEngine::NoWaiting;
    PlayEngine::State state = PlayEngine::Stopped;
    PlayEngine::ActivationState hwacc = PlayEngine::Unavailable;
    PlayEngine::Snapshot snapshot = PlayEngine::NoSnapshot;

    Mrl mrl;
    MrlState params, default_;
    QMutex mutex;

    struct {
        MediaObject media;
        VideoObject video; QTimer frameTimer; int delayed = 0;
        AudioObject audio;
        SubtitleObject subtitle;
        QVector<EditionChapterObject*> chapters, editions;
        EditionChapterObject chapter, edition;
    } info;

    MetaData metaData;

    HistoryModel *history = nullptr;
    YleDL *yle = nullptr;
    YouTubeDL *youtube = nullptr;

    struct {
        bool caching = false;
        int start = -1;
        QSharedPointer<MrlState> local;
    } t; // thread local

    bool hasImage = false, seekable = false, hasVideo = false;
    bool pauseAfterSkip = false, resume = false, hwdec = false;
    bool quit = false, preciseSeeking = false;

    QByteArray hwcdc;

    struct { int size = 0, used = 0; } cache;

    int avSync = 0, reload = -1;
    int time_s = 0, begin_s = 0, end_s = 0, duration_s = 0;
    int duration = 0, begin = 0, time = 0;

    QMap<QString, EncodingInfo> assEncodings;

    std::array<StreamData, StreamUnknown> streams = []() {
        std::array<StreamData, StreamUnknown> strs;
        strs[StreamVideo] = { "vid", VideoExt };
        strs[StreamAudio] = { "aid", AudioExt };
        strs[StreamSubtitle] = { "sid", SubtitleExt };
        return strs;
    }();


    struct {
        quint64 drawn = 0, dropped = 0, delayed = 0;
        SpeedMeasure<quint64> measure{5, 20};
    } frames;

    struct { QImage screen, video; } ss;
    QPoint mouse;


    auto updateState(State s) -> void;
    auto setWaitings(Waitings w, bool set) -> void;
    auto clearTimings() -> void;
    auto setInclusiveSubtitles(const QVector<SubComp> &loaded) -> void
        { setInclusiveSubtitles(&params, loaded); }
    auto setInclusiveSubtitles(MrlState *s, const QVector<SubComp> &loaded) -> void
        { sr->setComponents(loaded); s->set_sub_tracks_inclusive(sr->toTrackList()); }
    auto syncInclusiveSubtitles() -> void
        { params.set_sub_tracks_inclusive(sr->toTrackList()); }
    static auto restoreInclusiveSubtitles(const StreamList &tracks,
        const EncodingInfo &enc = EncodingInfo(), double acc = -1) -> QVector<SubComp>;
    auto audio_add(const QString &file, bool select) -> void
        { mpv.tellAsync("audio_add", MpvFile(file), select ? "select"_b : "auto"_b); }
    auto sub_add(const QString &file, const EncodingInfo &enc, bool select) -> void;
    auto autoselect(const MrlState *s, QVector<SubComp> &loads) -> void;
    auto autoloadFiles(StreamType type) -> MpvFileList;
    auto autoloadSubtitle(const MrlState *s) -> T<MpvFileList, QVector<SubComp>>;

    auto af(const MrlState *s) const -> QByteArray;
    auto vf(const MrlState *s) const -> QByteArray;
    auto vo(const MrlState *s) const -> QByteArray;
    auto videoSubOptions(const MrlState *s) const -> QByteArray;
    auto updateVideoSubOptions() -> void;
    auto renderVideoFrame(OpenGLFramebufferObject *fbo) -> void;
    auto displaySize() const { return info.video.output()->size(); }
    auto post(State state) -> void { _PostEvent(p, StateChange, state); }
    auto post(Waitings w, bool set) -> void { _PostEvent(p, WaitingChange, w, set); }

    auto volume(const MrlState *s) const -> double
        { return s->audio_volume() * 100 * s->audio_amplifier() * 100 * 1e-3; }

    auto loadfile(const Mrl &mrl, bool resume) -> void;
    auto updateMediaName(const QString &name = QString()) -> void;

    auto toTracks(const QVariant &var) -> QVector<StreamList>;
    auto refresh() -> void {mpv.tellAsync("frame_step"); mpv.tell("frame_back_step");}
    auto observe() -> void;
    auto process(QEvent *event) -> void;
    auto hook() -> void;
    auto setMousePos(const QPointF &pos)
    {
        if (!mpv.handle() || !params.d->disc)
            return false;
        if (!_Change(mouse, vr->mapToVideo(pos).toPoint()))
            return false;
        mpv.tellAsync("mouse", mouse.x(), mouse.y());
        return true;
    }
    auto takeSnapshot() -> void;
    auto localCopy() -> QSharedPointer<MrlState>;
    auto onLoad() -> void;
    auto onUnload() -> void;
    auto request() -> void;

    static auto detect(const StreamTrack &track, const EncodingInfo &enc, double acc) -> EncodingInfo
    {
        return detect(track.file(), enc.isValid() ? enc : track.encoding(), acc);
    }
    // acc < 0: fb (disable chardet)
    // acc = 0: autodetect no fallback
    // acc > 0: autodetect with fallback
    static auto detect(const QString &file, const EncodingInfo &fb, double acc) -> EncodingInfo
    {
        if (acc < 0)
            return fb;
        const auto enc = CharsetDetector::detect(file, acc);
        return enc.isValid() ? enc : fb;
    }
    static auto detect(const QString &file, const MrlState *s) -> EncodingInfo
        { return detect(file, s->d->subtitleEncoding, s->d->autodetect); }

    auto setSubtitleFiles(const QVector<SubtitleWithEncoding> &subs) -> void;
    auto addSubtitleFiles(const QVector<SubtitleWithEncoding> &subs) -> void;
};

#endif // PLAYENGINE_P_HPP
