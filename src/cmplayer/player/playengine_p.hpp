#ifndef PLAYENGINE_P_HPP
#define PLAYENGINE_P_HPP

#include "playengine.hpp"
#include "mpv_helper.hpp"
#include "tmp/type_test.hpp"
#include "tmp/type_info.hpp"
#include "misc/log.hpp"
#include "misc/tmp.hpp"
#include "misc/dataevent.hpp"
#include "audio/audiocontroller.hpp"
#include "video/videooutput.hpp"
#include "video/hwacc.hpp"
#include "video/deintoption.hpp"
#include "video/videoformat.hpp"
#include "video/videorendereritem.hpp"
#include "video/videofilter.hpp"
#include "video/videocolor.hpp"
#include "subtitle/submisc.hpp"
#include "subtitle/subtitlestyle.hpp"
#include "enum/deintmode.hpp"
#include "enum/colorspace.hpp"
#include "enum/colorrange.hpp"
#include "enum/audiodriver.hpp"
#include "enum/channellayout.hpp"
#include "enum/interpolatortype.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include <libmpv/client.h>

DECLARE_LOG_CONTEXT(Engine)

enum EventType {
    UserType = QEvent::User, UpdateTimeRange, UpdateTrackList, StateChange,
    PreparePlayback, UpdateChapterList, Tick, EndPlayback, StartPlayback,
    UpdateCache, UpdateCurrentStream, UpdateCurrentChapter,
    UpdateVideoInfo, UpdateAudioInfo, NotifySeek, UpdateMetaData
};

enum EndReason {
    EndFailed = -1, EndOfFile, EndRestart, EndRequest, EndQuit, EndUnknown
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

struct PlayEngine::Data {
    Data(PlayEngine *engine);
    PlayEngine *p = nullptr;
    Thread thread{p};
    AvInfoObject *videoInfo = new AvInfoObject, *audioInfo = new AvInfoObject;
    ChapterInfoObject *chapterInfo = nullptr;

    MediaInfoObject mediaInfo;
    HardwareAcceleration hwacc = HardwareAcceleration::Unavailable;
    MetaData metaData;
    QString mediaName;

    double fps = 1.0;
    bool hasImage = false, tempoScaler = false, seekable = false;
    bool subStreamsVisible = true, startPaused = false, disc = false;
    bool sgInit = false, glInit = false;

    AudioController *audio = nullptr;
    bool quit = false, timing = false, muted = false, initialized = false;
    int volume = 100;
    double amp = 1.0, speed = 1.0, avsync = 0;
    int cacheForPlayback = 20, cacheForSeeking = 50;
    int cache = -1.0, initSeek = -1;
    mpv_handle *handle = nullptr;
    VideoOutput *video = nullptr;
    VideoFilter *filter = nullptr;
    QByteArray hwaccCodecs;
    QVector<SubtitleFileInfo> subtitleFiles;
    ChannelLayout layout = ChannelLayoutInfo::default_();
    int duration = 0, audioSync = 0, begin = 0, position = 0;
    int subDelay = 0, chapter = -2, edition = -1;
    QVector<int> streamIds = {0, 0, 0}, lastStreamIds = streamIds;
    QVector<StreamList> streams = {StreamList(), StreamList(), StreamList()};
    AudioTrackInfoObject *audioTrackInfo = nullptr;
    SubtitleStyle subStyle;
    VideoRenderer *renderer = nullptr;
    ChapterList chapters, chapterFakeList;
    EditionList editions;

    HwAcc::Type hwaccBackend = HwAcc::None;
    VideoFormat videoFormat;
    DeintOption deint_swdec, deint_hwdec;
    DeintMode deint = DeintMode::Auto;
    QString audioDevice = u"auto"_q;

    StartInfo startInfo, nextInfo;

    SubtitleTrackInfoObject subtitleTrackInfo;

    auto af() const -> QByteArray;
    auto vf() const -> QByteArray;
    auto vo() const -> QByteArray;

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

    auto loadfile(const Mrl &mrl, int resume, int cache, int edition) -> void;
    auto loadfile() -> void { loadfile(startInfo.resume); }
    auto updateMediaName(const QString &name = QString()) -> void;
    template <class T>
    auto setmpv_async(const char *name, const T &value) -> void;
    template <class T>
    auto setmpv(const char *name, const T &value) -> void;
    template<class T>
    auto getmpv(const char *name, const T &def = T()) -> T;
    auto refresh() -> void {tellmpv("frame_step"); tellmpv("frame_back_step");}
    static auto error(int err) -> const char* { return mpv_error_string(err); }
    auto isSuccess(int error) -> bool { return error == MPV_ERROR_SUCCESS; }

    template<class... Args>
    auto check(int err, const char *msg, const Args &... args) -> bool;
    template<class... Args>
    auto fatal(int err, const char *msg, const Args &... args) -> void;
};

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
auto PlayEngine::Data::getmpv(const char *name, const T &def) -> T
{
    using trait = mpv_format_trait<T>;
    mpv_type<T> data;
    if (!handle || !check(mpv_get_property(handle, name,
                                           trait::format, &data),
                          "Couldn't get property '%%'.", name))
        return def;
    if (!trait::use_free)
        return trait::cast(data);
    T t = trait::cast(data);
    trait::free(data);
    return t;
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
        Log::write(getLogContext(), lv, "Error %%: %%", error(err),
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
