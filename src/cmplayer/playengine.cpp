#include "playengine.hpp"
#include "playengine_p.hpp"
#include "translator.hpp"
#include "log.hpp"
#include "tmp.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include "video/videoformat.hpp"
#include "video/videorendereritem.hpp"
#include "video/videofilter.hpp"
#include "video/videocolor.hpp"
#include "subtitle/submisc.hpp"
#include "subtitle/subtitlestyle.hpp"
#include "enum/interpolatortype.hpp"
#include "enum/colorrange.hpp"
#include "enum/channellayout.hpp"
#include "enum/deintmode.hpp"
#include "enum/audiodriver.hpp"
#include <libmpv/client.h>

DECLARE_LOG_CONTEXT(Engine)

enum EndReason {
    EndFailed = -1,
    EndOfFile,
    EndRestart,
    EndRequest,
    EndQuit,
    EndUnknown
};

template<class T, bool number = std::is_arithmetic<T>::value>
struct mpv_format_trait { };

template<>
struct mpv_format_trait<bool> {
    using mpv_type = int;
    static constexpr mpv_format format = MPV_FORMAT_FLAG;
    static constexpr bool use_free = false;
    static constexpr auto cast(mpv_type from) -> bool { return from; }
    static auto userdata(bool v) -> QByteArray { return QByteArray::number(v); }
    static auto free(mpv_type&) -> void { }
};

template<class T>
struct mpv_format_trait<T, true> {
    static constexpr bool IsInt = tmp::is_integral<T>();
    static constexpr bool use_free = false;
    using mpv_type = tmp::conditional_t<IsInt, qint64, double>;
    static_assert(!std::is_same<T, bool>::value, "wrong type");
    static constexpr mpv_format format = IsInt ? MPV_FORMAT_INT64
                                               : MPV_FORMAT_DOUBLE;
    static constexpr auto cast(mpv_type from) -> T { return from; }
    static auto userdata(T v) -> QByteArray { return QByteArray::number(v); }
    static auto free(mpv_type&) -> void { }
};

template<>
struct mpv_format_trait<QString> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static constexpr bool use_free = true;
    static auto userdata(mpv_type v) -> QByteArray { return v; }
    static auto cast(const char *from) -> QString
        { return QString::fromLocal8Bit(from); }
    static auto free(mpv_type &data) -> void { mpv_free((void*)data); }
};

template<>
struct mpv_format_trait<const char *> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static constexpr bool use_free = true;
    static auto userdata(mpv_type v) -> QByteArray { return v; }
    static auto cast(const char *from) -> const char* { return from; }
    static auto free(mpv_type &data) -> void { mpv_free((void*)data); }
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

class OptionList {
public:
    OptionList(char join = ',')
        : m_join(join) { }
    auto add(const char *key, const char *value, bool quote = false) -> void
        { add(key, QByteArray::fromRawData(value, qstrlen(value)), quote); }
    auto add(const char *key, const QByteArray &value,
             bool quote = false) -> void
    {
        if (!m_data.isEmpty())
            m_data.append(m_join);
        m_data.append(key);
        m_data.append('=');
        if (quote)
            m_data.append('"');
        m_data.append(value);
        if (quote)
            m_data.append('"');
    }
    auto add(const char *key, void *value) -> void
        { add(key, QByteArray::number((quint64)(quintptr)value)); }
    auto add(const char *key, double value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const char *key, int value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const char *key, bool value) -> void
        { add(key, value ? "yes" : "no"); }
    auto get() const -> const QByteArray& { return m_data; }
    auto data() const -> const char* { return m_data.data(); }
private:
    QByteArray m_data;
    char m_join;
};

template<class T>
using mpv_type = typename mpv_format_trait<T>::mpv_type;

struct PlayEngine::Data {
    template <class T>
    auto setmpv_async(const char *name, const T &value) -> void
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
    auto setmpv(const char *name, const T &value) -> void
    {
        if (handle) {
            mpv_type<T> data = value;
            check(mpv_set_property(handle, name, mpv_format_trait<T>::format,
                                   &data),
                  "Error on %%=%%", name, value);
        }
    }

    template<class T>
    auto getmpv(const char *name, const T &def = T()) -> T
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

    auto refresh() -> void
    {
        tellmpv("frame_step");
        tellmpv("frame_back_step");
    }

    Data(PlayEngine *engine)
        : p(engine) { }
    PlayEngine *p = nullptr;

    ImagePlayback image;
    InterpolatorType videoChromaUpscaler = InterpolatorType::Bilinear;

    MediaInfoObject mediaInfo;
    AvInfoObject *videoInfo = new AvInfoObject, *audioInfo = new AvInfoObject;
    HardwareAcceleration hwacc = HardwareAcceleration::Unavailable;
    MetaData metaData;
    QThread *videoThread = nullptr; // video decoding thread
    OpenGLOffscreenContext *videoContext = nullptr;
    QString mediaName;

    VideoColor videoEq;
    ColorRange videoColorRange = ColorRange::Auto;

    double fps = 1.0;
    bool hasImage = false, tempoScaler = false, seekable = false;
    bool subStreamsVisible = true, startPaused = false, disc = false;

    static auto error(int err) -> const char* { return mpv_error_string(err); }

    auto isSuccess(int error) -> bool { return error == MPV_ERROR_SUCCESS; }

    template<class... Args>
    auto check(int err, const char *msg, const Args &... args) -> bool
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
    auto fatal(int err, const char *msg, const Args &... args) -> void
    {
        if (!isSuccess(err))
            Log::write(getLogContext(), Log::Fatal, "Error %%: %%", error(err),
                       Log::parse(msg, args...));
    }

    auto setOption(const char *name, const char *data) -> void
    {
        const auto err = mpv_set_option_string(handle, name, data);
        fatal(err, "Couldn't set option %%=%%.", name, data);
    }

    Thread thread{p};
    AudioController *audio = nullptr;
    QTimer imageTicker;
    bool quit = false, timing = false, muted = false;
    int volume = 100;
    double amp = 1.0, speed = 1.0, avsync = 0;
    int cacheForPlayback = 20, cacheForSeeking = 50;
    int cache = -1.0, initSeek = -1;
    mpv_handle *handle = nullptr;
    VideoOutput *video = nullptr;
    VideoFilter *filter = nullptr;
    QByteArray hwaccCodecs;
    QList<SubtitleFileInfo> subtitleFiles;
    ChannelLayout layout = ChannelLayoutInfo::default_();
    int duration = 0, audioSync = 0, begin = 0, position = 0;
    int subDelay = 0, chapter = -2;
    QVector<int> streamIds = {0, 0, 0}, lastStreamIds = streamIds;
    QVector<StreamList> streams = {StreamList(), StreamList(), StreamList()};
    AudioTrackInfoObject *audioTrackInfo = nullptr;
    SubtitleStyle subStyle;
    VideoRendererItem *renderer = nullptr;
    ChapterList chapters, chapterFakeList;
    EditionList editions;
    int edition = -1;
    ChapterInfoObject *chapterInfo = nullptr;
    HwAcc::Type hwaccBackend = HwAcc::None;
    VideoFormat videoFormat;
    DeintOption deint_swdec, deint_hwdec;
    DeintMode deint = DeintMode::Auto;
    QByteArray ao = "";
    AudioDriver audioDriver = AudioDriver::Auto;

    StartInfo startInfo, nextInfo;

    SubtitleTrackInfoObject subtitleTrackInfo;

    auto af() const -> QByteArray
    {
        OptionList af(':');
        af.add("dummy:address", audio);
        af.add("use_scaler", (int)tempoScaler);
        af.add("layout", (int)layout);
        return af.get();
    }
    auto vf() const -> QByteArray
    {
        OptionList vf(':');
        vf.add("noformat:address", filter);
        vf.add("swdec_deint", deint_swdec.toString().toLatin1());
        vf.add("hwdec_deint", deint_hwdec.toString().toLatin1());
        return vf.get();
    }
    auto vo() const -> QByteArray
    {
        OptionList vo(':');
        vo.add("null:address", video);
        return vo.get();
    }

    auto mpVolume() const -> double { return volume*amp/10.0; }
    auto tellmpv(const QByteArray &cmd) -> void
    {
        if (handle)
            check(mpv_command_string(handle, cmd.constData()),
                  "Cannaot execute: %%", cmd);
    }
    auto tellmpv_async(const QByteArray &cmd,
                       std::initializer_list<QByteArray> &&list) -> void
    {
        QVector<const char*> args(list.size()+2, nullptr);
        auto it = args.begin();
        *it++ = cmd.constData();
        for (auto &one : list)
            *it++ = one.constData();
        if (handle)
            check(mpv_command_async(handle, 0, args.data()),
                  "Cannot execute: %%", cmd);
    }
    auto tellmpv(const QByteArray &cmd,
                 std::initializer_list<QByteArray> &&list) -> void
    {
        QVector<const char*> args(list.size()+2, nullptr);
        auto it = args.begin();
        *it++ = cmd.constData();
        for (auto &one : list)
            *it++ = one.constData();
        if (handle)
            check(mpv_command(handle, args.data()), "Cannot execute: %%", cmd);
    }
    template<class... Args>
    auto tellmpv(const QByteArray &cmd, const Args &... args) -> void
        { tellmpv(cmd, {qbytearray_from(args)...}); }
    template<class... Args>
    auto tellmpv_async(const QByteArray &cmd, const Args &... args) -> void
        { tellmpv_async(cmd, {qbytearray_from(args)...}); }

    auto updateMrl() -> void
    {
        hasImage = startInfo.mrl.isImage();
        updateMediaName();
        emit p->mrlChanged(startInfo.mrl);
    }

    auto loadfile(const Mrl &mrl, int resume, int cache, int edition) -> void
    {
        QString file = mrl.isLocalFile() ? mrl.toLocalFile() : mrl.toString();
        if (file.isEmpty())
            return;
        timing = false;
        OptionList opts;
        opts.add("ao", ao.isEmpty() ? "\"\"" : ao);
        if (hwaccCodecs.isEmpty() || hwaccBackend == HwAcc::None)
            opts.add("hwdec", "no");
        else {
            opts.add("hwdec", HwAcc::backendName(hwaccBackend).toLatin1());
            opts.add("hwdec-codecs", hwaccCodecs, true);
        }

        if (mrl.isDisc()) {
            file = mrl.titleMrl(edition >= 0 ? edition : -1).toString();
            initSeek = resume;
        } else {
            if (edition >= 0)
                opts.add("edition", edition);
            if (resume > 0)
                opts.add("start", resume/1000.0);
            initSeek = -1;
        }
        opts.add("deinterlace", deint != DeintMode::None);
        opts.add("volume", mpVolume());
        opts.add("mute", muted);
        opts.add("audio-delay", audioSync/1000.0);
        opts.add("sub-delay", subDelay/1000.0);

        const auto &font = subStyle.font;
        opts.add("sub-text-color", font.color.name(QColor::HexArgb).toLatin1());
        QStringList fontStyles;
        if (font.bold())
            fontStyles.append("Bold");
        if (font.italic())
            fontStyles.append("Italic");
        QString family = font.family();
        if (!fontStyles.isEmpty())
            family += ":style=" + fontStyles.join(' ');
        double factor = font.size;
        if (font.scale == OsdScalePolicy::Width)
            factor *= 1280;
        else if (font.scale == OsdScalePolicy::Diagonal)
            factor *= sqrt(1280*1280 + 720*720);
        else
            factor *= 720.0;
        opts.add("sub-text-font", family.toLatin1(), true);
        opts.add("sub-text-font-size", factor);
        const auto &outline = subStyle.outline;
        const auto scaled = [factor] (double v)
            { return qBound(0., v*factor, 10.); };
        const auto color = [] (const QColor &color)
            { return color.name(QColor::HexArgb).toLatin1(); };
        if (outline.enabled) {
            opts.add("sub-text-border-size", scaled(outline.width));
            opts.add("sub-text-border-color", color(outline.color));
        } else
            opts.add("sub-text-border-size", 0.0);
        const auto &bbox = subStyle.bbox;
        if (bbox.enabled)
            opts.add("sub-text-back-color", color(bbox.color));
        auto norm = [] (const QPointF &p)
            { return sqrt(p.x()*p.x() + p.y()*p.y()); };
        const auto &shadow = subStyle.shadow;
        if (shadow.enabled) {
            opts.add("sub-text-shadow-color", color(shadow.color));
            opts.add("sub-text-shadow-offset", scaled(norm(shadow.offset)));
        } else
            opts.add("sub-text-shadow-offset", 0.0);

        if (cache > 0) {
            opts.add("cache", cache);
            QByteArray value = "no";
            if (cacheForPlayback > 0)
                value.setNum(qMax<int>(1, cacheForPlayback*0.01));
            opts.add("cache-pause", value);
            opts.add("cache-min", cacheForPlayback);
            opts.add("cache-seek-min", cacheForSeeking);
        } else
            opts.add("cache", "no");
        opts.add("pause", p->isPaused() || hasImage);
        opts.add("audio-channels", ChannelLayoutInfo::data(layout), true);
        opts.add("af", af(), true);
        opts.add("vf", vf(), true);
        opts.add("brightness", videoEq.brightness());
        opts.add("contrast", videoEq.contrast());
        opts.add("hue", videoEq.hue());
        opts.add("saturation", videoEq.saturation());
        _Debug("Load: %% (%%)", file, opts.get());
        tellmpv("loadfile", file.toLocal8Bit(), "replace", opts.get());
    }
    auto loadfile(int resume) -> void
    {
        if (startInfo.isValid())
            loadfile(startInfo.mrl, resume, startInfo.cache, startInfo.edition);
    }
    auto loadfile() -> void { loadfile(startInfo.resume); }
    auto updateMediaName(const QString &name = QString()) -> void
    {
        mediaName = name;
        QString category;
        auto mrl = p->mrl();
        if (mrl.isLocalFile())
            category = tr("File");
        else if (mrl.isDvd())
            category = _L("DVD");
        else if (mrl.isBluray())
            category = tr("Blu-ray");
        else
            category = _L("URL");
        const QString display = name.isEmpty() ? mrl.displayName() : name;
        mediaInfo.setName(category % _L(": ") % display);
    }
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
    _Debug("Create audio/video plugins");
    d->audio = new AudioController(this);
    d->video = new VideoOutput(this);
    d->filter = new VideoFilter;

    d->chapterInfo = new ChapterInfoObject(this, this);
    d->audioTrackInfo = new AudioTrackInfoObject(this, this);
    d->imageTicker.setInterval(20);
    d->updateMediaName();

    _Debug("Make registrations and connections");

    connect(&d->imageTicker, &QTimer::timeout, [this] () {
        bool begin = false, duration = false, pos = false;
        if (d->hasImage) {
            pos = _Change(d->position, d->image.pos());
            begin = _Change(d->duration, d->image.duration());
            duration = _Change(d->begin, 0);
        }
        if (pos)
            emit tick(d->position);
        if (begin)
            emit beginChanged(d->begin);
        if (duration)
            emit durationChanged(d->duration);
        if (begin || duration)
            emit endChanged(end());
        if (pos || begin || duration)
            emit rateChanged();
    });
    connect(d->video, &VideoOutput::formatChanged,
            this, &PlayEngine::updateVideoFormat);
    connect(d->video, &VideoOutput::droppedFramesChanged,
            this, &PlayEngine::droppedFramesChanged);

    d->handle = mpv_create();
    auto verbose = qgetenv("CMPLAYER_MPV_VERBOSE").toLower();
    if (!verbose.isEmpty())
        mpv_request_log_messages(d->handle, verbose.constData());
    d->setOption("fs", "no");
    d->setOption("input-cursor", "yes");
    d->setOption("softvol", "yes");
    d->setOption("softvol-max", "1000.0");
    d->setOption("sub-auto", "no");
    d->setOption("osd-level", "0");
    d->setOption("quiet", "yes");
    d->setOption("input-terminal", "no");
    d->setOption("ao", "null,");
    d->setOption("ad-lavc-downmix", "no");
    d->setOption("title", "\"\"");
    d->setOption("vo", d->vo().constData());
    d->setOption("fixed-vo", "yes");

    mpv_set_wakeup_callback(d->handle, [] (void *arg) {
        if (!static_cast<Data*>(arg)->videoThread)
            static_cast<Data*>(arg)->videoThread = QThread::currentThread();
    }, d);

    auto overrides = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
    if (!overrides.isEmpty()) {
        const auto opts = QString::fromLocal8Bit(overrides);
        const auto args = opts.split(QRegularExpression(R"([\s\t]+)"),
                                     QString::SkipEmptyParts);
        for (int i=0; i<args.size(); ++i) {
            if (!args[i].startsWith("--")) {
                _Error("Cannot parse option %%.", args[i]);
                continue;
            }
            const auto arg = args[i].midRef(2);
            const int index = arg.indexOf('=');
            if (index < 0) {
                if (arg.startsWith("no-"))
                    d->setOption(arg.mid(3).toLatin1(), "no");
                else
                    d->setOption(arg.toLatin1(), "yes");
            } else {
                const auto key = arg.left(index).toLatin1();
                const auto value = arg.mid(index+1).toLatin1();
                d->setOption(key, value);
            }
        }
    }
    d->fatal(mpv_initialize(d->handle), "Couldn't initialize mpv.");
    _Debug("Initialized");
}

PlayEngine::~PlayEngine()
{
    delete d->videoInfo;
    delete d->audioInfo;
    delete d->chapterInfo;
    delete d->audioTrackInfo;
    delete d->audio;
    delete d->video;
    delete d->filter;
    mpv_destroy(d->handle);
    delete d;
    _Debug("Finalized");
}

auto PlayEngine::initializeGL(QQuickWindow *window) -> void
{
    auto ctx = window->openglContext();
    ctx->doneCurrent();
    d->videoContext = new OpenGLOffscreenContext;
    d->videoContext->setShareContext(ctx);
    d->videoContext->setFormat(ctx->format());
    if (d->videoThread)
        d->videoContext->setThread(d->videoThread);
    d->videoContext->create();
    ctx->makeCurrent(window);
    d->filter->initializeGL(d->videoContext);
    d->video->initializeGL(d->videoContext);
}

auto PlayEngine::finalizeGL() -> void
{
    d->video->finalizeGL();
    d->filter->finalizeGL();
    _Delete(d->videoContext);
}

auto PlayEngine::metaData() const -> const MetaData&
{
    return d->metaData;
}

auto PlayEngine::updateVideoFormat(VideoFormat format) -> void
{
    if (_Change(d->videoFormat, format))
        emit videoFormatChanged(d->videoFormat);
    auto output = d->videoInfo->m_output;
    output->setBps(d->videoFormat.bitrate(output->m_fps));
}

auto PlayEngine::subtitleTrackInfo() const -> SubtitleTrackInfoObject*
{
    return &d->subtitleTrackInfo;
}

auto PlayEngine::setSubtitleDelay(int ms) -> void
{
    if (_Change(d->subDelay, ms))
        d->setmpv_async("sub-delay", d->subDelay/1000.0);
}

auto PlayEngine::setSubtitleTracks(const QStringList &tracks) -> void
{
    d->subtitleTrackInfo.set(tracks);
    emit subtitleTrackInfoChanged();
}

auto PlayEngine::setCurrentSubtitleIndex(int idx) -> void
{
    d->subtitleTrackInfo.setCurrentIndex(idx);
}

auto PlayEngine::chapterInfo() const -> ChapterInfoObject*
{
    return d->chapterInfo;
}

auto PlayEngine::audioTrackInfo() const -> AudioTrackInfoObject*
{
    return d->audioTrackInfo;
}

auto PlayEngine::mediaName() const -> QString
{
    return d->mediaName;
}

auto PlayEngine::cache() const -> qreal
{
    return d->cache/100.0;
}

auto PlayEngine::begin() const -> int
{
    return d->begin;
}

auto PlayEngine::end() const -> int
{
    return d->begin + d->duration;
}

auto PlayEngine::setImageDuration(int duration) -> void
{
    d->image.setDuration(duration);
}

auto PlayEngine::duration() const -> int
{
    return d->duration;
}

auto PlayEngine::currentEdition() const -> int
{
    return d->edition;
}

auto PlayEngine::editions() const -> const EditionList&
{
    return d->editions;
}

auto PlayEngine::chapters() const -> const ChapterList&
{
    return d->chapters;
}

auto PlayEngine::subtitleStreams() const -> const StreamList&
{
    return d->streams[Stream::Subtitle];
}

auto PlayEngine::videoRenderer() const -> VideoRendererItem*
{
    return d->renderer;
}

auto PlayEngine::videoStreams() const -> const StreamList&
{
    return d->streams[Stream::Video];
}

auto PlayEngine::audioSync() const -> int
{
    return d->audioSync;
}

auto PlayEngine::audioStreams() const -> const StreamList&
{
    return d->streams[Stream::Audio];
}

auto PlayEngine::hwAcc() const -> PlayEngine::HardwareAcceleration
{
    return d->hwacc;
}

auto PlayEngine::run() -> void
{
    d->thread.start();
}

auto PlayEngine::thread() const -> QThread*
{
    return &d->thread;
}

auto PlayEngine::waitUntilTerminated() -> void
{
    if (d->thread.isRunning())
        d->thread.wait();
}

auto PlayEngine::speed() const -> double
{
    return d->speed;
}

auto PlayEngine::setSpeed(double speed) -> void
{
    if (_ChangeZ(d->speed, speed)) {
        d->setmpv_async("speed", speed);
        emit speedChanged(d->speed);
    }
}

auto PlayEngine::setSubtitleStyle(const SubtitleStyle &style) -> void
{
    d->subStyle = style;
}

auto PlayEngine::seek(int pos) -> void
{
    d->chapter = -1;
    if (d->hasImage)
        d->image.seek(pos, false);
    else
        d->tellmpv("seek", (double)pos/1000.0, 2);
}

auto PlayEngine::relativeSeek(int pos) -> void
{
    if (d->hasImage)
        d->image.seek(pos, true);
    else
        d->tellmpv("seek", (double)pos/1000.0, 0);
    emit sought();
}

auto PlayEngine::setClippingMethod(ClippingMethod method) -> void
{
    d->audio->setClippingMethod(method);
}

auto PlayEngine::setChannelLayoutMap(const ChannelLayoutMap &map) -> void
{
    d->audio->setChannelLayoutMap(map);
}

auto PlayEngine::reload() -> void
{
    d->startInfo.edition = d->edition;
    d->loadfile(d->position);
}

auto PlayEngine::setChannelLayout(ChannelLayout layout) -> void
{
    if (_Change(d->layout, layout) && d->position > 0)
        reload();
}

using AudioDriverName = QPair<AudioDriver, const char*>;
const std::array<AudioDriverName, AudioDriverInfo::size()-1> audioDriverNames =
{{
    {AudioDriver::ALSA, "alsa"},
    {AudioDriver::OSS, "oss"},
    {AudioDriver::PulseAudio, "pulse"},
    {AudioDriver::CoreAudio, "coreaudio"},
    {AudioDriver::PortAudio, "portaudio"},
    {AudioDriver::JACK, "jack"},
    {AudioDriver::OpenAL, "openal"}
}};

auto PlayEngine::setAudioDriver(AudioDriver driver) -> void
{
    if (_Change(d->audioDriver, driver)) {
        auto it = _FindIf(audioDriverNames,
                          [driver] (const AudioDriverName &one)
                              { return one.first == driver; });
        d->ao = it != audioDriverNames.end() ? it->second : "";
    }
}

auto PlayEngine::screen() const -> QQuickItem*
{
    return d->renderer;
}

auto PlayEngine::setMinimumCache(int playback, int seeking) -> void
{
    d->cacheForPlayback = playback;
    d->cacheForSeeking = seeking;
}

auto PlayEngine::volumeNormalizer() const -> double
{
    auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
}

auto PlayEngine::setHwAcc(int backend, const QList<int> &codecs) -> void
{
    for (auto id : codecs) {
        if (const char *name = HwAcc::codecName(id)) {
            d->hwaccCodecs.append(name);
            d->hwaccCodecs.append(',');
        }
    }
    d->hwaccCodecs.chop(1);
    d->hwaccBackend = HwAcc::Type(backend);
}

bool PlayEngine::isSubtitleStreamsVisible() const {return d->subStreamsVisible;}

auto PlayEngine::setSubtitleStreamsVisible(bool visible) -> void
{
    d->subStreamsVisible = visible;
    const auto id = currentSubtitleStream();
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id >= 0));
}

auto PlayEngine::setCurrentSubtitleStream(int id) -> void
{
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id > 0));
    if (id > 0)
        d->setmpv_async("sub", id);
}

auto PlayEngine::currentSubtitleStream() const -> int
{
    if (d->streams[Stream::Subtitle].isEmpty())
        return 0;
    return d->streamIds[Stream::Subtitle];
}

auto PlayEngine::addSubtitleStream(const QString &fileName,
                                   const QString &enc) -> bool
{
    QFileInfo info(fileName);
    for (auto &file : d->subtitleFiles)
        if (file.path == info.absoluteFilePath())
            return false;
    if (info.exists()) {
        SubtitleFileInfo file;
        file.path = info.absoluteFilePath();
        file.encoding = enc;
        d->subtitleFiles.append(file);
        d->setmpv("options/subcp", enc.toLatin1().constData());
        d->tellmpv("sub_add", file.path);
        if (d->subStreamsVisible)
            d->setmpv_async("sub-visibility", true);
        return true;
    }
    return false;
}

auto PlayEngine::removeSubtitleStream(int id) -> void
{
    auto &streams = d->streams[Stream::Subtitle];
    auto it = streams.find(id);
    if (it != streams.end()) {
        if (it->isExternal()) {
            for (int i=0; i<d->subtitleFiles.size(); ++i) {
                if (d->subtitleFiles[i].path == it->m_fileName)
                    d->subtitleFiles.removeAt(i);
            }
        }
        d->tellmpv("sub_remove", id);
    }
}

auto PlayEngine::avgfps() const -> double
{
    return d->video->avgfps();
}

auto PlayEngine::avgsync() const -> double
{
    return d->avsync;
}

auto PlayEngine::setNextStartInfo(const StartInfo &startInfo) -> void
{
    d->nextInfo = startInfo;
}

auto PlayEngine::stepFrame(int direction) -> void
{
    if ((m_state & (Playing | Paused)) && d->seekable)
        d->tellmpv_async(direction > 0 ? "frame_step" : "frame_back_step");
}

auto PlayEngine::updateState(State state) -> void
{
    const bool wasRunning = isRunning();
    if (_Change(m_state, state)) {
        emit stateChanged(m_state);
        if (m_state & (Playing | Paused)) {
            if (d->hasImage)
                d->imageTicker.start();
        } else {
            if (d->hasImage)
                d->imageTicker.stop();
        }
        if (wasRunning != isRunning())
            emit runningChanged();
    }
}

template<class T>
static auto _CheckSwap(T &the, T &one) -> bool {
    if (the == one)
        return false;
    the.swap(one);
    return true;
}

auto PlayEngine::customEvent(QEvent *event) -> void
{
    switch ((int)event->type()) {
    case UpdateChapterList: {
        auto chapters = _GetData<ChapterList>(event);
        if (_CheckSwap(d->chapters, chapters)) {
            d->chapterInfo->setCount(d->chapters.size());
            emit chaptersChanged(d->chapters);
            if (!d->chapters.isEmpty()) {
                Chapter prev, last;
                prev.m_id = -1;
                prev.m_time = _Min<int>();
                last.m_id = d->chapters.last().id() + 1;
                last.m_time = _Max<int>();
                d->chapterFakeList.append(prev);
                d->chapterFakeList += d->chapters;
                d->chapterFakeList.append(last);
            } else
                d->chapterFakeList.clear();
        }
        break;
    } case UpdateCache:
        d->cache = _GetData<int>(event);
        emit cacheChanged();
        break;
    case UpdateCurrentStream: {
        const auto ids = _GetData<QVector<int>>(event);
        Q_ASSERT(ids.size() == 3);
        auto check = [&] (Stream::Type type, void (PlayEngine::*sig)(int)) {
            const int id = ids[type];
            if (!_Change(d->streamIds[type], id))
                return;
            auto &streams = d->streams[type];
            for (auto it = streams.begin(); it != streams.end(); ++it)
                it->m_selected = it->id() == id;
            emit (this->*sig)(id);
        };
        check(Stream::Audio, &PlayEngine::currentAudioStreamChanged);
        check(Stream::Video, &PlayEngine::currentVideoStreamChanged);
        check(Stream::Subtitle, &PlayEngine::currentSubtitleStreamChanged);
        break;
    } case UpdateTrackList: {
        d->lastStreamIds = d->streamIds;
        auto streams = _GetData<QVector<StreamList>>(event);
        Q_ASSERT(streams.size() == 3);
        auto check = [&] (Stream::Type type,
                          void (PlayEngine::*sig)(const StreamList&))
        {
            auto &_streams = d->streams[type];
            if (!_CheckSwap(_streams, streams[type]))
                return false;
            d->streamIds[type] = -1;
            for (auto it = _streams.begin(); it != _streams.end(); ++it) {
                if (it->isSelected()) {
                    d->streamIds[type] = it->id();
                    break;
                }
            }
            emit (this->*sig)(_streams);
            return true;
        };
        if (check(Stream::Video, &PlayEngine::videoStreamsChanged))
            emit hasVideoChanged();
        if (check(Stream::Audio, &PlayEngine::audioStreamsChanged)) {
            d->audioTrackInfo->setCount(d->streams[Stream::Audio].size());
            d->audioTrackInfo->setCurrent(d->streamIds[Stream::Audio]);
        }
        check(Stream::Subtitle, &PlayEngine::subtitleStreamsChanged);
        break;
    } case StateChange:
        updateState(_GetData<PlayEngine::State>(event));
        break;
    case PreparePlayback: {
        d->subtitleFiles.clear();
        break;
    } case StartPlayback: {
        d->video->reset();
        QString name; bool seekable = false;
        _GetAllData(event, name, seekable, d->editions);
        if (_Change(d->seekable, seekable))
            emit seekableChanged(d->seekable);
        emit audioChanged();
        emit cacheChanged();
        int title = -1;
        for (auto &item : d->editions) {
            if (item.isSelected())
                title = item.id();
        }
        d->edition = title;
        emit editionsChanged(d->editions);
        if (!d->getmpv<bool>("pause"))
            updateState(Playing);
        emit started(d->startInfo.mrl);
        d->updateMediaName(name);
        d->lastStreamIds = d->streamIds;
        break;
    } case EndPlayback: {
        Mrl mrl; EndReason reason; _GetAllData(event, mrl, reason);
        int remain = (d->duration + d->begin) - d->position;
        d->nextInfo = StartInfo();
        auto state = Stopped;
        switch (reason) {
        case EndOfFile:
            _Info("Playback reached end-of-file");
            emit requestNextStartInfo();
            if (d->nextInfo.isValid())
                state = Loading;
            remain = 0;
            break;
        case EndQuit: case EndRequest:
            _Info("Playback has been terminated by request");
            break;
        default:
            _Info("Playback has been terminated by error(s)");
            state = Error;
        }
        updateState(state);
        if (state != Error && !mrl.isEmpty()) {
            FinishInfo info;
            info.mrl = mrl;
            info.position = d->position;
            info.remain = remain;
            info.streamIds = d->lastStreamIds;
            emit finished(info);
        }
        if (d->nextInfo.isValid())
            load(d->nextInfo);
        else if (_Change(d->seekable, false))
            emit seekableChanged(d->seekable);
        break;
    } case UpdateTimeRange:
        tie(d->begin, d->duration) = _GetData<int, int>(event);
        emit durationChanged(d->duration);
        emit beginChanged(d->begin);
        emit endChanged(end());
        break;
    case Tick: {
        _GetAllData(event, d->position, d->avsync);
        emit tick(d->position);
        emit rateChanged();
        break;
    } case UpdateCurrentChapter:
        if (_Change(d->chapter, _GetData<int>(event)))
            emit currentChapterChanged(d->chapter);
        break;
    case UpdateAudioInfo: {
        delete d->audioInfo;
        d->audioInfo = _GetData<AvInfoObject*>(event);
        emit audioChanged();
        break;
    } case UpdateVideoInfo: {
        delete d->videoInfo;
        d->videoInfo = _GetData<AvInfoObject*>(event);
        auto output = d->videoInfo->m_output;
        output->m_bitrate = d->videoFormat.bitrate(output->m_fps);
        auto hwacc = [&] () {
            auto codec = HwAcc::codecId(d->videoInfo->codec().toLatin1());
            if (!HwAcc::supports(d->hwaccBackend, codec))
                return HardwareAcceleration::Unavailable;
            static QVector<QString> types = {
                _L("vaapi"), _L("vdpau"), _L("vda")
            };
            if (types.contains(output->m_type))
                return HardwareAcceleration::Activated;
            if (d->hwaccCodecs.contains(d->videoInfo->codec().toLatin1()))
                return HardwareAcceleration::Unavailable;
            return HardwareAcceleration::Deactivated;
        };
        auto hwtxt = [] (HardwareAcceleration hwacc) {
            switch (hwacc) {
            case HardwareAcceleration::Activated:
                return tr("Activated");
            case HardwareAcceleration::Deactivated:
                return tr("Deactivated");
            default:
                return tr("Unavailable");
            }
        };
        if (_Change(d->hwacc, hwacc()))
            emit hwaccChanged();
        d->videoInfo->m_hwacc = hwtxt(d->hwacc);
        emit videoChanged();
        if (_Change(d->fps, output->m_fps))
            emit fpsChanged(d->fps);
        break;
    } case NotifySeek:
        emit sought();
        break;
    case UpdateMetaData: {
        d->metaData = _GetData<MetaData>(event);
        emit metaDataChanged();
        break;
    } default:
        break;
    }
}

auto PlayEngine::mediaInfo() const -> MediaInfoObject*
{
    return &d->mediaInfo;
}

auto PlayEngine::audioInfo() const -> AvInfoObject*
{
    return d->audioInfo;
}

auto PlayEngine::videoInfo() const -> AvInfoObject*
{
    return d->videoInfo;
}

auto PlayEngine::setState(PlayEngine::State state) -> void
{
    _PostEvent(this, StateChange, state);
}

auto PlayEngine::setCurrentChapter(int id) -> void
{
    d->setmpv_async("chapter", id);
}

auto PlayEngine::setCurrentEdition(int id, int from) -> void
{
    const auto mrl = d->startInfo.mrl;
    if (id == DVDMenu && mrl.isDisc()) {
        static const char *cmds[] = {"discnav", "menu", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds),
                 "Couldn't send 'discnav menu'.");
    } else if (0 <= id && id < d->editions.size()) {
        d->setmpv(mrl.isDisc() ? "disc-title" : "edition", id);
        seek(from);
    }
}

auto PlayEngine::setVolume(int volume) -> void
{
    if (_Change(d->volume, qBound(0, volume, 100))) {
        d->setmpv_async("volume", d->mpVolume());
        emit volumeChanged(d->volume);
    }
}

auto PlayEngine::isMuted() const -> bool
{
    return d->muted;
}

auto PlayEngine::volume() const -> int
{
    return d->volume;
}

auto PlayEngine::amp() const -> double
{
    return d->amp;
}

auto PlayEngine::setAmp(double amp) -> void
{
    if (_ChangeZ(d->amp, qBound(0.0, amp, 10.0))) {
        d->setmpv_async("volume", d->mpVolume());
        emit preampChanged(d->amp);
    }
}

auto PlayEngine::setMuted(bool muted) -> void
{
    if (_Change(d->muted, muted)) {
        d->setmpv_async("mute", d->muted);
        emit mutedChanged(d->muted);
    }
}

auto PlayEngine::exec() -> void
{
    _Debug("Start playloop thread");
    d->quit = false;
    int position = 0, cache = -1, duration = 0;
    bool first = false, loaded = false;
    Mrl mrl;
    QByteArray leftmsg;

    auto metaData = [&] () {
        auto list = d->getmpv<QVariant>("metadata").toList();
        MetaData metaData;
        for (int i=0; i+1<list.size(); i+=2) {
            const auto key = list[i].toString();
            const auto value = list[i+1].toString();
            if (key == _L("title"))
                metaData.m_title = value;
            else if (key == _L("artist"))
                metaData.m_artist = value;
            else if (key == _L("album"))
                metaData.m_album = value;
            else if (key == _L("genre"))
                metaData.m_genre = value;
            else if (key == _L("date"))
                metaData.m_date = value;
        }
        metaData.m_mrl = mrl;
        metaData.m_duration = duration;
        return metaData;
    };

    auto time = [] (double s) -> int { return s*1000 + 0.5; };

    auto checkTime = [&] () {
        if (_Change(position, time(d->getmpv<double>("time-pos")))
                && position > 0) {
            if (first) {
                duration = time(d->getmpv<double>("length"));
                const auto start = time(d->getmpv<double>("time-start"));
                _PostEvent(this, UpdateTimeRange, start, duration);
                const auto array = d->getmpv<QVariant>("chapter-list").toList();
                ChapterList chapters; chapters.resize(array.size());
                for (int i=0; i<array.size(); ++i) {
                    const auto map = array[i].toMap();
                    auto &chapter = chapters[i];
                    chapter.m_id = i;
                    chapter.m_time = time(map["time"].toDouble());
                    chapter.m_name = map["title"].toString();
                    if (chapter.m_name.isEmpty())
                        chapter.m_name = _MSecToString(chapter.m_time,
                                                       _L("hh:mm:ss.zzz"));
                }
                _PostEvent(this, UpdateChapterList, chapters);
                _PostEvent(this, UpdateMetaData, metaData());
                first = false;
            }
            double sync = 0;
            if (d->isSuccess(mpv_get_property(d->handle, "avsync",
                                              MPV_FORMAT_DOUBLE, &sync))
                    && d->renderer)
                sync *= 1000.0;
            _PostEvent(this, Tick, position, sync);
        }
    };

    auto reason = [&loaded] (void *data) {
        const auto reason = static_cast<mpv_event_end_file*>(data)->reason;
        if (reason > EndUnknown)
            return EndUnknown;
        if (reason == EndOfFile && !loaded)
            return EndFailed;
        return static_cast<EndReason>(reason);
    };

    while (!d->quit) {
        const auto event = mpv_wait_event(d->handle, 0.005);
        switch (event->event_id) {
        case MPV_EVENT_NONE: {
            if (!d->timing)
                break;
            checkTime();
            if (position > 0 && cache >= 0) {
                qint64 newCache = -1;
                const auto res = mpv_get_property(d->handle, "cache",
                                                  MPV_FORMAT_INT64, &newCache);
                switch (res) {
                case MPV_ERROR_SUCCESS:
                    break;
                default:
                    newCache = -1;
                    if (res != MPV_ERROR_PROPERTY_UNAVAILABLE)
                        _Error("Error for cache: %%", mpv_error_string(res));
                }
                if (_Change(cache, (int)newCache))
                    _PostEvent(this, UpdateCache, cache);
            }
            break;
        } case MPV_EVENT_LOG_MESSAGE: {
            auto message = static_cast<mpv_event_log_message*>(event->data);
            leftmsg += message->text;
            int from = 0;
            for (;;) {
                auto to = leftmsg.indexOf('\n', from);
                if (to < 0)
                    break;
                Log::write(Log::Info, "[mpv/%%] %%", message->prefix,
                           leftmsg.mid(from, to-from));
                from = to + 1;
            }
            leftmsg = leftmsg.mid(from);
            break;
        } case MPV_EVENT_IDLE:
            break;
        case MPV_EVENT_START_FILE:
            loaded = false;
            position = -1;
            cache = 0;
            mrl = d->startInfo.mrl;
            _PostEvent(this, StateChange, Loading);
            _PostEvent(this, PreparePlayback);
            break;
        case MPV_EVENT_FILE_LOADED: {
            loaded = true;
            d->timing = first = true;
            d->disc = mrl.isDisc();
            if (d->initSeek > 0) {
                d->tellmpv("seek", d->initSeek, 2);
                d->initSeek = -1;
            }
            const char *listprop = d->disc ? "disc-titles" : "editions";
            const char *itemprop = d->disc ? "disc-title"  : "edition";
            EditionList editions;
            auto add = [&] (int id) -> Edition& {
                auto &title = editions[id];
                title.m_id = id;
                title.m_name = tr("Title %1").arg(id+1);
                return title;
            };
            const int list = d->getmpv<int>(listprop, 0);
            editions.resize(list);
            for (int i=0; i<list; ++i)
                add(i);
            if (list > 0) {
                const int item = d->getmpv<int>(itemprop);
                if (0 <= item && item < list)
                    editions[item].m_selected = true;
            }
            const auto name = d->getmpv<QString>("media-title");
            const auto seekable = d->getmpv<bool>("seekable", false);
            _PostEvent(this, StartPlayback, name, seekable, editions);
            break;
        } case MPV_EVENT_END_FILE: {
            d->disc = d->timing = false;
            _PostEvent(this, EndPlayback, mrl, reason(event->data));
            break;
        } case MPV_EVENT_CHAPTER_CHANGE:
            _PostEvent(this, UpdateCurrentChapter, d->getmpv<int>("chapter"));
            break;
        case MPV_EVENT_TRACKS_CHANGED: {
            QVector<StreamList> streams(3);
            auto list = d->getmpv<QVariant>("track-list").toList();
            for (auto &var : list) {
                auto map = var.toMap();
                auto type = Stream::Unknown;
                switch (map["type"].toString().at(0).unicode()) {
                case 'v': type = Stream::Video; break;
                case 'a': type = Stream::Audio; break;
                case 's': type = Stream::Subtitle; break;
                default: continue;
                }
                Stream stream;
                stream.m_type = type;
                stream.m_albumart = map["albumart"].toBool();
                stream.m_codec = map["codec"].toString();
                stream.m_default = map["default"].toBool();
                stream.m_id = map["id"].toInt();
                stream.m_lang = map["lang"].toString();
                if (_InRange(2, stream.m_lang.size(), 3)
                        && _IsAlphabet(stream.m_lang))
                    stream.m_lang = Translator::displayLanguage(stream.m_lang);
                stream.m_title = map["title"].toString();
                stream.m_fileName = map["external-filename"].toString();
                if (!stream.m_fileName.isEmpty())
                    stream.m_title = QFileInfo(stream.m_fileName).fileName();
                stream.m_selected = map["selected"].toBool();
                streams[stream.m_type].insert(stream.m_id, stream);
            }
            _PostEvent(this, UpdateTrackList, streams);
            break;
        } case MPV_EVENT_TRACK_SWITCHED: {
            QVector<int> ids(3, 0);
            ids[Stream::Video] = d->getmpv<int>("vid");
            ids[Stream::Audio] = d->getmpv<int>("aid");
            ids[Stream::Subtitle] = d->getmpv<int>("sid");
            _PostEvent(this, UpdateCurrentStream, ids);
            break;
        } case MPV_EVENT_PAUSE:
        case MPV_EVENT_UNPAUSE: {
            const auto paused = d->getmpv<bool>("core-idle");
            const auto byCache = d->getmpv<bool>("paused-for-cache");
            const auto state = byCache ? Buffering : paused ? Paused : Playing;
            _PostEvent(this, StateChange, state);
            break;
        } case MPV_EVENT_SET_PROPERTY_REPLY:
            if (!d->isSuccess(event->error)) {
                auto ptr = reinterpret_cast<void*>(event->reply_userdata);
                auto data = static_cast<QByteArray*>(ptr);
                _Debug("Error %%: Couldn't set property %%.",
                       mpv_error_string(event->error), *data);
                delete data;
            }
            break;
        case MPV_EVENT_GET_PROPERTY_REPLY: {
            break;
        } case MPV_EVENT_AUDIO_RECONFIG: {
            auto audio = new AvInfoObject;
            audio->m_driver = AudioDriverInfo::name(d->audioDriver);
            audio->m_codec = d->getmpv<QString>("audio-format");
            audio->m_codecDescription = d->getmpv<QString>("audio-codec");

            const auto in = d->audio->inputFormat();
            const auto out = d->audio->outputFormat();
            audio->m_input->m_bitrate = d->getmpv<int>("audio-bitrate")*8;
            audio->m_input->m_samplerate
                    = d->getmpv<int>("audio-samplerate")/1000.0;
            audio->m_input->m_channels = in.channels();
            audio->m_input->m_bits = in.bits();
            audio->m_input->m_type = in.type();

            audio->m_output->m_bitrate = out.bitrate();
            audio->m_output->m_samplerate = out.samplerate();
            audio->m_output->m_channels = out.channels();
            audio->m_output->m_bits = out.bits();
            audio->m_output->m_type = out.type();

            _PostEvent(this, UpdateAudioInfo, audio);
            break;
        } case MPV_EVENT_VIDEO_RECONFIG: {
            auto video = new AvInfoObject;
            auto vin = d->getmpv<QVariant>("video-params").toMap();
            auto vout = d->getmpv<QVariant>("video-out-params").toMap();
            video->m_input->m_bitrate = d->getmpv<int>("video-bitrate")*8;
            video->m_input->m_type = vin["pixelformat"].toString();
            video->m_input->m_size.rwidth() = vin["w"].toInt();
            video->m_input->m_size.rheight() = vin["h"].toInt();
            video->m_input->m_fps = d->getmpv<double>("fps");
            video->m_output->m_type = vout["pixelformat"].toString();
            video->m_output->m_size.rwidth() = vout["dw"].toInt();
            video->m_output->m_size.rheight() = vout["dh"].toInt();
            video->m_output->m_fps = d->getmpv<double>("fps");
            video->m_codecDescription = d->getmpv<QString>("video-codec");
            video->m_codec = d->getmpv<QString>("video-format");
            video->moveToThread(qApp->instance()->thread());
            _PostEvent(this, UpdateVideoInfo, video);
            break;
        } case MPV_EVENT_SHUTDOWN:
            d->quit = true;
            break;
        case MPV_EVENT_PLAYBACK_RESTART:
            checkTime();
            _PostEvent(this, NotifySeek);
            break;
        case MPV_EVENT_METADATA_UPDATE:
            _PostEvent(this, UpdateMetaData, metaData());
            break;
        default:
            break;
        }
    }
    _Debug("Finish playloop thread");
}

auto PlayEngine::shutdown() -> void
{
    d->tellmpv("quit 1");
}

auto PlayEngine::startInfo() const -> const StartInfo&
{
    return d->startInfo;
}

auto PlayEngine::load(const StartInfo &info) -> void
{
    const bool changed = d->startInfo.mrl != info.mrl;
    d->startInfo = info;
    if (changed)
        d->updateMrl();
    if (info.isValid())
        d->loadfile();
}

auto PlayEngine::time() const -> int
{
    return d->position;
}

auto PlayEngine::isSeekable() const -> bool
{
    return d->seekable;
}

auto PlayEngine::hasVideo() const -> bool
{
    return !d->streams[Stream::Video].isEmpty();
}

auto PlayEngine::currentChapter() const -> int
{
    return d->chapter;
}

auto PlayEngine::pause() -> void
{
    if (d->hasImage)
        setState(PlayEngine::Paused);
    else
        d->setmpv("pause", true);
}

auto PlayEngine::unpause() -> void
{
    if (d->hasImage)
        setState(PlayEngine::Playing);
    else
        d->setmpv("pause", false);
}

auto PlayEngine::mrl() const -> Mrl
{
    return d->startInfo.mrl;
}

auto PlayEngine::currentAudioStream() const -> int
{
    return d->streamIds[Stream::Audio];
}

auto PlayEngine::setCurrentVideoStream(int id) -> void
{
    if (d->streams[Stream::Video].contains(id))
        d->setmpv_async("video", id);
}

auto PlayEngine::currentVideoStream() const -> int
{
    return hasVideo() ? d->streamIds[Stream::Video] : 0;
}

auto PlayEngine::setCurrentAudioStream(int id) -> void
{
    if (d->streams[Stream::Audio].contains(id))
        d->setmpv_async("audio", id);
}

auto PlayEngine::setAudioSync(int sync) -> void
{
    if (_Change(d->audioSync, sync))
        d->setmpv_async("audio-delay", sync*0.001);
}

auto PlayEngine::fps() const -> double
{
    return d->fps;
}

auto PlayEngine::videoChromaUpscaler() const -> InterpolatorType
{
    return d->videoChromaUpscaler;
}

auto PlayEngine::setVideoChromaUpscaler(InterpolatorType type) -> void
{
    if (_Change(d->videoChromaUpscaler, type))
        emit videoChromaUpscalerChanged(type);
}

auto PlayEngine::setVideoColorRange(ColorRange range) -> void
{
    if (_Change(d->videoColorRange, range))
        emit videoColorRangeChanged(range);
}

auto PlayEngine::videoColorRange() const -> ColorRange
{
    return d->videoColorRange;
}

auto PlayEngine::setVideoEqualizer(const VideoColor &prop) -> void
{
    d->videoEq = prop;
    d->setmpv_async("brightness", prop.brightness());
    d->setmpv_async("contrast", prop.contrast());
    d->setmpv_async("hue", prop.hue());
    d->setmpv_async("saturation", prop.saturation());
}

auto PlayEngine::videoEqualizer() const -> const VideoColor&
{
    return d->videoEq;
}

auto PlayEngine::setVideoRenderer(VideoRendererItem *renderer) -> void
{
    if (d->renderer != renderer) {
        if (d->renderer) {
            disconnect(d->renderer, nullptr, this, nullptr);
            disconnect(d->renderer, nullptr, d->filter, nullptr);
            disconnect(d->filter, nullptr, d->renderer, nullptr);
        }
        d->renderer = renderer;
        d->video->setRenderer(d->renderer);
    }
}

auto PlayEngine::droppedFrames() const -> int
{
    return d->video->droppedFrames();
}

auto PlayEngine::bitrate(double fps) const -> double
{
    return d->videoFormat.bitrate(fps);
}

auto PlayEngine::videoFormat() const -> VideoFormat
{
    return d->videoFormat;
}

auto PlayEngine::registerObjects() -> void
{
    qRegisterMetaType<PlayEngine::State>("State");
    qRegisterMetaType<Mrl>("Mrl");
    qRegisterMetaType<VideoFormat>("VideoFormat");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<StreamList>("StreamList");
    qRegisterMetaType<AudioFormat>("AudioFormat");
    qmlRegisterType<ChapterInfoObject>();
    qmlRegisterType<AudioTrackInfoObject>();
    qmlRegisterType<SubtitleTrackInfoObject>();
    qmlRegisterType<AvInfoObject>();
    qmlRegisterType<AvIoFormat>();
    qmlRegisterType<MediaInfoObject>();
    qmlRegisterType<PlayEngine>("CMPlayer", 1, 0, "Engine");
}

auto PlayEngine::setVolumeNormalizerActivated(bool on) -> void
{
    if (d->audio->isNormalizerActivated() != on) {
        d->audio->setNormalizerActivated(on);
        emit volumeNormalizerActivatedChanged(on);
    }
}

auto PlayEngine::setTempoScalerActivated(bool on) -> void
{
    if (_Change(d->tempoScaler, on)) {
        d->tellmpv("af", "set", d->af());
        emit tempoScaledChanged(on);
    }
}

auto PlayEngine::isVolumeNormalizerActivated() const -> bool
{
    return d->audio->isNormalizerActivated();
}

auto PlayEngine::isTempoScaled() const -> bool
{
    return d->audio->isTempoScalerActivated();
}

auto PlayEngine::stop() -> void
{
    d->tellmpv("stop");
}

auto PlayEngine::setVolumeNormalizerOption(const AudioNormalizerOption &option)
-> void
{
    d->audio->setNormalizerOption(option);
}

auto PlayEngine::setDeintOptions(const DeintOption &swdec,
                                 const DeintOption &hwdec) -> void
{
    d->deint_swdec = swdec;
    d->deint_hwdec = hwdec;
    emit deintOptionsChanged();
}

auto PlayEngine::deintOptionForSwDec() const -> DeintOption
{
    return d->deint_swdec;
}

auto PlayEngine::deintOptionForHwDec() const -> DeintOption
{
    return d->deint_hwdec;
}

auto PlayEngine::setDeintMode(DeintMode mode) -> void
{
    if (_Change(d->deint, mode)) {
        if (isPaused()) {
            d->setmpv("deinterlace", !!(int)mode);
            d->refresh();
        } else
            d->setmpv_async("deinterlace", !!(int)mode);
    }
}

auto PlayEngine::deintMode() const -> DeintMode
{
    return d->deint;
}

auto PlayEngine::stateText(State state) -> QString
{
    switch (state) {
    case Playing:
        return tr("Playing");
    case Stopped:
        return tr("Stopped");
    case Loading:
        return tr("Loading");
    case Buffering:
        return tr("Buffering");
    case Error:
        return tr("Error");
    default:
        return tr("Paused");
    }
}

auto PlayEngine::sendMouseClick(const QPointF &pos) -> void
{
    if (d->handle && d->disc) {
        m_mouse = pos.toPoint();
        static const char *cmds[] = {"discnav", "mouse", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds), "Couldn't send mouse.");
    }
}

auto PlayEngine::sendMouseMove(const QPointF &pos) -> void
{
    if (d->handle && d->disc && _Change(m_mouse, pos.toPoint())) {
        static const char *cmds[] = {"discnav", "mouse_move", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds),
                 "Couldn't send mouse_move.");
    }
}

auto PlayEngine::subtitleFiles() const -> QList<SubtitleFileInfo>
{
    return d->subtitleFiles;
}
