#ifndef PLAYENGINE_P_HPP
#define PLAYENGINE_P_HPP

#include "playengine.hpp"
#include "mpv_helper.hpp"
#include "streamtrack.hpp"
#include "tmp/type_test.hpp"
#include "tmp/type_info.hpp"
#include "misc/log.hpp"
#include "misc/tmp.hpp"
#include "misc/dataevent.hpp"
#include "misc/youtubedl.hpp"
#include "audio/audiocontroller.hpp"
#include "audio/audioformat.hpp"
#include "video/videooutput.hpp"
#include "video/hwacc.hpp"
#include "video/deintoption.hpp"
#include "video/videoformat.hpp"
#include "video/videorenderer.hpp"
#include "video/videofilter.hpp"
#include "video/videocolor.hpp"
#include "subtitle/submisc.hpp"
#include "misc/osdtheme.hpp"
#include "misc/speedmeasure.hpp"
#include "enum/deintmode.hpp"
#include "enum/colorspace.hpp"
#include "enum/colorrange.hpp"
#include "enum/audiodriver.hpp"
#include "enum/channellayout.hpp"
#include "enum/interpolator.hpp"
#include "enum/dithering.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include "opengl/opengllogger.hpp"
#include <libmpv/client.h>
#include <libmpv/opengl_cb.h>
#include <functional>
#include "avinfoobject.hpp"

DECLARE_LOG_CONTEXT(Engine)

enum EventType {
    UserType = QEvent::User, StateChange,
    PreparePlayback,EndPlayback, StartPlayback, NotifySeek,
    EventTypeMax
};

enum MpvState {
    MpvStopped, MpvLoading, MpvRunning
};

static constexpr const int UpdateEventBegin = QEvent::User + 1000;

struct StreamTypeInfo {
    StreamType type;
    const char *mpvName;
    void (PlayEngine::*notifyList)(const StreamList&);
    void (PlayEngine::*notifyTrack)(int);
};

static const QVector<StreamTypeInfo> streamTypeInfo = [] () {
    QVector<StreamTypeInfo> info(3);
    info[StreamVideo] = {
        StreamVideo, "vid",
        &PlayEngine::videoStreamsChanged,
        &PlayEngine::currentVideoStreamChanged
    };
    info[StreamAudio] = {
        StreamAudio, "aid",
        &PlayEngine::audioStreamsChanged,
        &PlayEngine::currentAudioStreamChanged
    };
    info[StreamSubtitle] = {
        StreamSubtitle, "sid",
        &PlayEngine::subtitleStreamsChanged,
        &PlayEngine::currentSubtitleStreamChanged
    };
    return info;
}();

static const QVector<StreamType> streamTypes
    = { StreamAudio, StreamVideo, StreamSubtitle };

struct Observation {
    int event;
    const char *name = nullptr;
    std::function<void(void)> post; // post from mpv to qt
    std::function<void(QEvent*)> handle; // handle posted event
};

extern auto initialize_vdpau() -> void;
extern auto finalize_vdpau() -> void;
extern auto initialize_vaapi() -> void;
extern auto finalize_vaapi() -> void;

class PlayEngine::Thread : public QThread {
public:
    Thread(PlayEngine *engine): engine(engine) {}
private:
    PlayEngine *engine = nullptr;
    auto run() -> void { engine->exec(); }
};

namespace detail {
struct mpv_format_string {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static constexpr bool use_free = true;
    static auto userdata(mpv_type v) -> QByteArray { return v; }
    static auto free(mpv_type &data) -> void { mpv_free((void*)data); }
};
}

template<class T, bool number = tmp::is_arithmetic<T>()>
struct mpv_format_trait { };

template<class T>
struct mpv_format_trait<T, true> {
    SCA IsBool = tmp::is_same<T, bool>();
    SCA IsInt = tmp::is_integral<T>();
    SCA use_free = false;
    using mpv_type = tmp::conditional_t<IsBool, int,
                     tmp::conditional_t<IsInt, qint64, double>>;
    static constexpr mpv_format format = IsBool ? MPV_FORMAT_FLAG
                                       : IsInt ? MPV_FORMAT_INT64
                                               : MPV_FORMAT_DOUBLE;
    static constexpr auto cast(mpv_type from) -> T { return from; }
    static auto userdata(T v) -> QByteArray { return QByteArray::number(v); }
    static auto free(mpv_type&) -> void { }
};

template<>
struct mpv_format_trait<QString> : detail::mpv_format_string {
    static auto cast(const char *from) -> QString
        { return QString::fromLocal8Bit(from); }
};

template<>
struct mpv_format_trait<QByteArray> : detail::mpv_format_string {
    static auto cast(const char *from) -> QByteArray
        { return QByteArray(from); }
};

template<>
struct mpv_format_trait<const char *> : detail::mpv_format_string {
    static auto cast(const char *from) -> const char* { return from; }
};

template<>
struct mpv_format_trait<QVariant> {
    using mpv_type = mpv_node;
    static constexpr mpv_format format = MPV_FORMAT_NODE;
    static constexpr bool use_free = true;
    static auto userdata(mpv_type) -> QByteArray { return "mpv-node"; }
    static auto cast(mpv_type v) -> QVariant { return parse(v); }
    static auto free(mpv_type &node) -> void { mpv_free_node_contents(&node); }
private:
    static QVariant parse(const mpv_node &node) {
        switch (node.format) {
        case MPV_FORMAT_DOUBLE:
            return node.u.double_;
        case MPV_FORMAT_FLAG:
            return !!node.u.flag;
        case MPV_FORMAT_INT64:
            return QVariant::fromValue<int>(node.u.int64);
        case MPV_FORMAT_STRING:
            return QString::fromLocal8Bit(node.u.string);
        case MPV_FORMAT_NODE_ARRAY: {
            auto array = node.u.list;
            QVariantList list; list.reserve(array->num);
            for (int i=0; i<array->num; ++i)
                list.append(parse(array->values[i]));
            return list;
        } case MPV_FORMAT_NODE_MAP: {
            auto list = node.u.list; QVariantMap map;
            for (int i=0; i<list->num; ++i) {
                auto data = parse(list->values[i]);
                map.insert(QString::fromLocal8Bit(list->keys[i]), data);
            }
            return map;
        } default:
            return QVariant();
        }
    }
};

template<class T>
using mpv_type = typename mpv_format_trait<T>::mpv_type;

struct StreamData {
    StreamList tracks;
    QStringList priority;
    int reserved = -1;
};

SCIA s2ms(double s) -> int { return s*1000 + 0.5; }

struct PlayEngine::Data {
    Data(PlayEngine *engine);
    PlayEngine *p = nullptr;
    Thread thread{p};
    QTemporaryDir confDir;
    QTemporaryFile customShader;
    QStringList audioPriorty;
    QVector<Observation> observations;

    VideoInfoObject videoInfo;
    AudioInfoObject audioInfo;
    SubtitleInfoObject subInfo;
    ChapterInfoObject *chapterInfo = nullptr;

    MediaInfoObject mediaInfo;
    ActivationState hwacc = Unavailable;
    MetaData metaData;
    QString mediaName;

    MpvState mpvState = MpvStopped;
    bool initOffscreen = false;
    OpenGLOffscreenContext offscreen;
    OpenGLFramebufferObject *fbo = nullptr;
    OpenGLLogger glLogger{"Offscreen"};

    double fps = 1.0;
    bool hasImage = false, tempoScaler = false, seekable = false, hasVideo = false;
    bool subStreamsVisible = true, startPaused = false, disc = false;

    AudioController *audio = nullptr;
    bool quit = false, muted = false, initialized = false;
    int volume = 100, avSync = 0;
    double amp = 1.0, speed = 1.0;
    qreal cacheForPlayback = 0.02, cacheForSeeking = 0.05;
    bool cacheEnabled = false;
    int cacheSize = 0, cacheUsed = 0, initSeek = -1;
    int updateEventMax = UpdateEventBegin;

    mpv_handle *handle = nullptr;
    QByteArray client;

//    VideoOutput *video = nullptr;
    VideoFilter *filter = nullptr;
    bool useHwAcc = false;
    QByteArray hwCodecs;
    QVector<SubtitleFileInfo> subtitleFiles;
    ChannelLayout layout = ChannelLayoutInfo::default_();
    int duration = 0, audioSync = 0, begin = 0, position = 0;
    int subDelay = 0, chapter = -2, edition = -1;
    QVector<StreamData> streams = {StreamData(), StreamData(), StreamData()};
    Mrl mpvMrl;
    mpv_opengl_cb_context *glMpv = nullptr;
    QMatrix4x4 c_matrix;
    VideoColor videoEq;
    VideoEffects videoEffects = 0;

    YouTubeDL *youtube = nullptr;

    OsdTheme subStyle;
    VideoRenderer *video = nullptr;
    ChapterList chapters;
    EditionList editions;

    VideoFormat videoFormat;
    DeintOption deint_swdec, deint_hwdec;
    DeintMode deint = DeintMode::Auto;
    QString audioDevice = u"auto"_q;

    StartInfo startInfo, nextInfo;

    Interpolator lscale = Interpolator::Bilinear, cscale = Interpolator::Bilinear;
    ColorSpace colorSpace = ColorSpace::Auto;
    ColorRange colorRange = ColorRange::Auto;
    Dithering dithering = Dithering::None;

    quint64 drawnFrames = 0, droppedFrames = 0, delayedFrames = 0;
    SpeedMeasure<quint64> fpsMeasure{5, 20};

    auto clearTimings()
    {
        fpsMeasure.reset();
        videoInfo.setDroppedFrames(0);
        videoInfo.setDelayedFrames(0);
        videoInfo.renderer()->setFps(0);
        drawnFrames = 0;
    }

    auto af() const -> QByteArray;
    auto vf() const -> QByteArray;
    auto vo() const -> QByteArray;
    auto videoSubOptions() const -> QByteArray;
    auto updateVideoSubOptions() -> void;
    auto updateColorMatrix() -> void;
    auto renderVideoFrame(OpenGLFramebufferObject *fbo) -> void;
    auto displaySize() const { return videoInfo.renderer()->size(); }
    auto postState(State state) -> void { _PostEvent(p, StateChange, state); }
    auto exec() -> void;

    auto mpVolume() const -> double { return volume*amp/10.0; }
    template<class T>
    SIA qbytearray_from(const T &t) -> tmp::enable_if_t<tmp::is_arithmetic<T>(),
        QByteArray> { return QByteArray::number(t); }
    SIA qbytearray_from(const QByteArray &t) -> QByteArray { return t; }
    SIA qbytearray_from(const QString &t) -> QByteArray { return t.toLocal8Bit(); }

    auto tellmpv(const QByteArray &cmd) -> void;
    auto tellmpv_async(const QByteArray &cmd,
                       std::initializer_list<QByteArray> &&list) -> void;
    auto tellmpv(const QByteArray &cmd,
                 std::initializer_list<QByteArray> &&list) -> void;
    template<class... Args>
    auto tellmpv(const QByteArray &cmd, const Args &... args) -> void
        { tellmpv(cmd, {qbytearray_from(args)...}); }
    template<class... Args>
    auto tellmpv_async(const QByteArray &cmd, const Args &... args) -> void
        { tellmpv_async(cmd, {qbytearray_from(args)...}); }

    auto updateMrl() -> void;
    auto loadfile(int resume) -> void;

    auto loadfile(const Mrl &mrl, int resume, int cachePercent, int edition) -> void;
    auto loadfile() -> void { loadfile(startInfo.resume); }
    auto updateMediaName(const QString &name = QString()) -> void;
    template <class T>
    auto setmpv_async(const char *name, const T &value) -> void;
    template <class T>
    auto setmpv(const char *name, const T &value) -> void;
    template<class T>
    auto getmpv(const char *name) -> T;
    template<class T>
    auto getmpv(const char *name, T &val) -> bool;
    auto refresh() -> void {tellmpv("frame_step"); tellmpv("frame_back_step");}
    static auto error(int err) -> const char* { return mpv_error_string(err); }
    auto isSuccess(int error) -> bool { return error == MPV_ERROR_SUCCESS; }
    template<class T>
    auto post(mpv_event *event, T &val, const T &fallback = T()) -> void;
    template<class... Args>
    auto check(int err, const char *msg, const Args &... args) -> bool;
    template<class... Args>
    auto fatal(int err, const char *msg, const Args &... args) -> void;
    auto getmpvosd(const char *name) -> QString {
        auto buf = mpv_get_property_osd_string(handle, name);
        auto ret = QString::fromLatin1(buf);
        mpv_free(buf);
        return ret;
    }

    auto observation(int event) -> const Observation&
    {
        Q_ASSERT(UpdateEventBegin <= event && event < updateEventMax);
        Q_ASSERT(event == observations[event - UpdateEventBegin].event);
        return observations[event - UpdateEventBegin];
    }
    auto newUpdateEvent() -> int { return updateEventMax++; }
    auto observe(const char *name, std::function<void(void)> &&post) -> int
    {
        const int event = newUpdateEvent();
        Observation ob;
        ob.event = event;
        ob.name = name;
        ob.post = post;
        ob.handle = [] (QEvent*) {};
        observations.append(ob);
        Q_ASSERT(observations.size() == updateEventMax - UpdateEventBegin);
        return event;
    }
    template<class Get>
    auto observe(const char *name, Get get,
                 std::function<void(QEvent*)> &&handle) -> int
    {
        const int event = newUpdateEvent();
        Observation ob;
        ob.event = event;
        ob.name = name;
        ob.post = [=] () { _PostEvent(p, event, get()); };
        ob.handle = handle;
        observations.append(ob);
        Q_ASSERT(observations.size() == updateEventMax - UpdateEventBegin);
        return event;
    }
    template<class T, class Set>
    auto observeType(const char *name, Set set) -> int
    {
        return observe(name, [=] () { return getmpv<T>(name); },
                       [=] (QEvent *e) { set(_MoveData<T>(e)); });
    }

    template<class T, class Get, class Notify>
    auto observe(const char *name, T &t, Get get, Notify notify) -> int
    {
        return observe<Get>(name, get, [=, &t] (QEvent *e) {
            if (t != _GetData<T>(e)) {
                _TakeData(e, t);
                if (initialized)
                    notify();
            }
        });
    }

    template<class T, class Get>
    auto observe(const char *name, T &t, Get get, void(PlayEngine::*sig)()) -> int
        { return observe<T, Get>(name, t, get, [=] () { emit (p->*sig)(); }); }

    template<class T, class Get, class S>
    auto observe(const char *name, T &t, Get get, void(PlayEngine::*sig)(S)) -> int
        { return observe<T, Get>(name, t, get, [=, &t] () { emit (p->*sig)(t); }); }

    template<class T>
    auto observe(const char *name, T &t, void(PlayEngine::*sig)()) -> int
    {
        return observe(name, t, [=, &t] () {
            T val = t; return getmpv<T>(name, val) ? val : T();
        }, sig);
    }

    template<class T, class S>
    auto observe(const char *name, T &t, void(PlayEngine::*sig)(S)) -> int
    {
        return observe(name, t, [=, &t] () {
            T val = t; return getmpv<T>(name, val) ? val : T();
        }, sig);
    }
    auto observeTime(const char *name, int &t, void(PlayEngine::*sig)(int)) -> int
    {
        return observe<int>(name, t, [=, &t] { return s2ms(getmpv<double>(name)); }, sig);
    }

    auto observeTrack(StreamType type) -> int
    {
        const auto &info = streamTypeInfo[type];
        return observe(info.mpvName, [=, &info] () {
            int track = 0; return getmpv<int>(info.mpvName, track) ? track : 1;
        }, [=] (QEvent *event) { setCurrentTrack(type, _GetData<int>(event)); });
    }
    auto setStreamList(StreamType type, StreamList &&list) -> bool;
    auto setCurrentTrack(StreamType type, int id) -> void
    {
        Q_ASSERT(type != StreamUnknown);
        if (id == currentTrack(type))
            return;
        for (auto &track : streams[type].tracks)
            track.m_selected = track.id() == id;
        emit (p->*(streamTypeInfo[type].notifyTrack))(id);
    }
    auto currentTrack(StreamType type) -> int
    {
        Q_ASSERT(type != StreamUnknown);
        return _FindSelectedTrackId(streams[type].tracks);
    }
    auto observe() -> void;
    auto dispatch(mpv_event *event) -> void;
    auto process(QEvent *event) -> void;
    auto log(const QByteArray &prefix, const QByteArray &text) -> void;
    auto translateMpvStateToState() -> void;
    int hookId = 0;
    template<class F>
    auto hook(const QByteArray &when, F &&handler) -> void
    {
        Q_ASSERT(!hooks.contains(when));
        tellmpv("hook_add", when, hookId++, 0);
        hooks[when] = std::move(handler);
    }
    auto hook() -> void;
    QMap<QByteArray, std::function<void(void)>> hooks;
};

template<class T>
auto PlayEngine::Data::post(mpv_event *event, T &val, const T &fb) -> void
{
    Q_ASSERT(event->event_id == MPV_EVENT_PROPERTY_CHANGE);
    const auto ev = static_cast<mpv_event_property*>(event->data);
    T newVal = val;
    if (!getmpv<T>(ev->name, newVal))
        newVal = fb;
    if (_Change(val, newVal))
        _PostEvent(p, event->reply_userdata, val);
}

template <class T>
auto PlayEngine::Data::setmpv_async(const char *name, const T &value) -> void
{
    if (handle) {
        mpv_type<T> data = value;
        auto userdata = new QByteArray(name);
        *userdata += "=" + mpv_format_trait<T>::userdata(value);
        check(mpv_set_property_async(handle, (quint64)(void*)userdata, name,
                                     mpv_format_trait<T>::format, &data),
              "Error on %%", *userdata);
    }
}

template <class T>
auto PlayEngine::Data::setmpv(const char *name, const T &value) -> void
{
    if (handle) {
        mpv_type<T> data = value;
        check(mpv_set_property(handle, name, mpv_format_trait<T>::format,
                               &data),
              "Error on %%=%%", name, value);
    }
}

template<class T>
auto PlayEngine::Data::getmpv(const char *name, T &def) -> bool
{
    using trait = mpv_format_trait<T>;
    mpv_type<T> data;
    if (!handle || !check(mpv_get_property(handle, name,
                                           trait::format, &data),
                          "Couldn't get property '%%'.", name))
        return false;
    def = trait::cast(data);
    if (trait::use_free)
        trait::free(data);
    return true;
}

template<class T>
auto PlayEngine::Data::getmpv(const char *name) -> T
{
    T t = T(); getmpv<T>(name, t); return t;
}

template<class... Args>
auto PlayEngine::Data::check(int err, const char *msg,
                             const Args &... args) -> bool
{
    if (isSuccess(err))
        return true;
    const auto lv = err == MPV_ERROR_PROPERTY_UNAVAILABLE ? Log::Debug
                                                          : Log::Error;
    if (lv <= Log::maximumLevel())
        Log::write(getLogContext(), lv, "%% %%: %%",
                   lv == Log::Debug ? "Debug" : "Error", error(err),
                   Log::parse(msg, args...));
    return false;
}

template<class... Args>
auto PlayEngine::Data::fatal(int err, const char *msg,
                             const Args &... args) -> void
{
    if (!isSuccess(err))
        Log::write(getLogContext(), Log::Fatal, "Error %%: %%", error(err),
                   Log::parse(msg, args...));
}

#endif // PLAYENGINE_P_HPP
