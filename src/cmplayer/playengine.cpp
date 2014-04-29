#include "playengine.hpp"
#include "playengine_p.hpp"
#include "translator.hpp"
#include "log.hpp"
#include "video/videorendereritem.hpp"
#include "subtitle/submisc.hpp"
#include "subtitle/subtitlestyle.hpp"

DECLARE_LOG_CONTEXT(Engine)
#include <libmpv/client.h>

enum EndReason {
    EndFailed = -1,
    EndOfFile,
    EndRestart,
    EndRequest,
    EndQuit,
    EndUnknown
};

template<class T, bool number = std::is_arithmetic<T>::value> struct mpv_format_trait { };
template<> struct mpv_format_trait<bool> {
    using mpv_type = int;
    static constexpr mpv_format format = MPV_FORMAT_FLAG;
    static constexpr bool use_free = false;
    static QByteArray userdata(bool v) { return QByteArray::number(v); }
    static inline constexpr bool cast(mpv_type from) { return from; }
    static inline void free(mpv_type&) {}
};
template<class T> struct mpv_format_trait<T, true> {
    static_assert(!std::is_same<T, bool>::value, "wrong type");
    static constexpr bool IsInt = std::is_integral<T>::value;
    using mpv_type = typename std::conditional<IsInt, qint64, double>::type;
    static constexpr bool use_free = false;
    static constexpr mpv_format format = IsInt ? MPV_FORMAT_INT64 : MPV_FORMAT_DOUBLE;
    static QByteArray userdata(T v) { return QByteArray::number(v); }
    static inline constexpr T cast(mpv_type from) { return from; }
    static inline void free(mpv_type&) {}
};
template<> struct mpv_format_trait<QString> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static constexpr bool use_free = true;
    static QByteArray userdata(mpv_type v) { return v; }
    static inline QString cast(const char *from) { return QString::fromLocal8Bit(from); }
    static inline void free(mpv_type &data) { mpv_free((void*)data); }
};
template<> struct mpv_format_trait<const char *> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static constexpr bool use_free = true;
    static QByteArray userdata(mpv_type v) { return v; }
    static inline const char *cast(const char *from) { return from; }
    static inline void free(mpv_type &data) { mpv_free((void*)data); }
};
template<> struct mpv_format_trait<QVariant> {
    using mpv_type = mpv_node;
    static constexpr mpv_format format = MPV_FORMAT_NODE;
    static constexpr bool use_free = true;
    static QByteArray userdata(mpv_type) { return "mpv-node"; }
    static inline QVariant cast(mpv_type v) { return parse(v); }
    static inline void free(mpv_type &node) { mpv_free_node_contents(&node); }
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
            for (int i=0; i<list->num; ++i)
                map.insert(QString::fromLocal8Bit(list->keys[i]), parse(list->values[i]));
            return map;
        } default:
            return QVariant();
        }
    }
};

class OptionList {
public:
    OptionList(char join = ','): m_join(join) {}
    void add(const char *key, const char *value, bool quote = false) {
        add(key, QByteArray::fromRawData(value, qstrlen(value)), quote);
    }
    void add(const char *key, const QByteArray &value, bool quote = false) {
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
    void add(const char *key, void *value) { add(key, QByteArray::number((quint64)(quintptr)value)); }
    void add(const char *key, double value) { add(key, QByteArray::number(value)); }
    void add(const char *key, int value) { add(key, QByteArray::number(value)); }
    void add(const char *key, bool value) { add(key, value ? "yes" : "no"); }
    const QByteArray &get() const { return m_data; }
    const char *data() const { return m_data.data(); }
private:
    QByteArray m_data; char m_join;
};

struct PlayEngine::Data {
    template <class T>
    void setmpv_async(const char *name, const T &value) {
        if (handle) {
            using trait = mpv_format_trait<T>;    typename trait::mpv_type data = value;
            auto userdata = new QByteArray(name); *userdata += "=" + trait::userdata(value);
            check(mpv_set_property_async(handle, (quint64)(void*)userdata, name, trait::format, &data), "Error on %%", *userdata);
        }
    }
    template <class T>
    void setmpv(const char *name, const T &value) {
        if (handle) {
            using trait = mpv_format_trait<T>; typename trait::mpv_type data = value;
            check(mpv_set_property(handle, name, trait::format, &data), "Error on %%=%%", name, value);
        }
    }
    template<class T>
    T getmpv(const char *name, const T &def = T()) {
        using trait = mpv_format_trait<T>; using type = typename trait::mpv_type; type data;
        if (!handle || !check(mpv_get_property(handle, name, trait::format, &data), "Couldn't get property '%%'.", name))
            return def;
        if (!trait::use_free)
            return trait::cast(data);
        T t = trait::cast(data); trait::free(data); return t;
    }

    Data(PlayEngine *engine): p(engine) {}
    PlayEngine *p = nullptr;

    ImagePlayback image;

    MediaInfoObject mediaInfo;
    AvInfoObject *videoInfo = new AvInfoObject, *audioInfo = new AvInfoObject;
    HardwareAcceleration hwacc = HardwareAcceleration::Unavailable;
    MetaData metaData;
    QString mediaName;

    double fps = 1.0;
    bool hasImage = false, tempoScaler = false, seekable = false;
    bool subStreamsVisible = true, startPaused = false, disc = false;

    static const char *error(int err) { return mpv_error_string(err); }
    bool isSuccess(int error) { return error == MPV_ERROR_SUCCESS; }
    template<class... Args>
    bool check(int err, const char *msg, const Args &... args) {
        if (isSuccess(err))
            return true;
        const auto lv = err == MPV_ERROR_PROPERTY_UNAVAILABLE ? Log::Debug : Log::Error;
        if (lv <= Log::maximumLevel())
            Log::write(getLogContext(), lv, "Error %%: %%", error(err), Log::parse(msg, args...));
        return false;
    }
    template<class... Args>
    void fatal(int err, const char *msg, const Args &... args) {
        if (!isSuccess(err))
            Log::write(getLogContext(), Log::Fatal, "Error %%: %%", error(err), Log::parse(msg, args...));
    }
    void setOption(const char *name, const char *data) {
        fatal(mpv_set_option_string(handle, name, data), "Couldn't set option %%=%%.", name, data);
    }

    Thread thread{p};
    AudioController *audio = nullptr;
    QTimer imageTicker;
    bool quit = false, timing = false, muted = false, tick = false;
    int volume = 100;
    double amp = 1.0, speed = 1.0, avsync = 0;
    int cacheForPlayback = 20, cacheForSeeking = 50;
    int cache = -1.0, initSeek = -1;
    mpv_handle *handle = nullptr;
    VideoOutput *video = nullptr;
    QByteArray hwaccCodecs;
    QList<SubtitleFileInfo> subtitleFiles;
    ChannelLayout layout = ChannelLayoutInfo::default_();
    int duration = 0, audioSync = 0, begin = 0, position = 0, subDelay = 0, chapter = -2;
    QVector<int> streamIds = {0, 0, 0}, lastStreamIds = streamIds;
    QVector<StreamList> streams = {StreamList(), StreamList(), StreamList()};
    AudioTrackInfoObject *audioTrackInfo = nullptr;
    SubtitleStyle subStyle;
    VideoRendererItem *renderer = nullptr;
    ChapterList chapters, chapterFakeList;
    EditionList editions;
    int edition = -1;
    ChapterInfoObject *chapterInfo = nullptr;
    QPoint mouse{-1, -1};
    QList<QMetaObject::Connection> rendererConnections;
    HwAcc::Type hwaccBackend = HwAcc::None;
    VideoFormat videoFormat;
    DeintOption deint_swdec, deint_hwdec;
    DeintMode deint = DeintMode::Auto;
    QByteArray ao = "";
    AudioDriver audioDriver = AudioDriver::Auto;

    StartInfo startInfo, nextInfo;

    SubtitleTrackInfoObject subtitleTrackInfo;

    QByteArray af() const {
        OptionList af(':');
        af.add("dummy:address", audio);
        af.add("use_scaler", (int)tempoScaler);
        af.add("layout", (int)layout);
        return af.get();
    }

    double mpVolume() const { return volume*amp/10.0; }
    void tellmpv(const QByteArray &cmd) {
        if (handle)
            check(mpv_command_string(handle, cmd.constData()), "Cannaot execute: %%", cmd);
    }
    void tellmpv_async(const QByteArray &cmd, std::initializer_list<QByteArray> &&list) {
        QVector<const char*> args(list.size()+2, nullptr);
        auto it = args.begin(); *it++ = cmd.constData(); for (auto &one : list) { *it++ = one.constData(); }
        if (handle)
            check(mpv_command_async(handle, 0, args.data()), "Cannot execute: %%", cmd);
    }
    void tellmpv(const QByteArray &cmd, std::initializer_list<QByteArray> &&list) {
        QVector<const char*> args(list.size()+2, nullptr);
        auto it = args.begin(); *it++ = cmd.constData(); for (auto &one : list) { *it++ = one.constData(); }
        if (handle)
            check(mpv_command(handle, args.data()), "Cannot execute: %%", cmd);
    }
    template<class... Args>
    void tellmpv(const QByteArray &cmd, const Args &... args) {
        tellmpv(cmd, {qbytearray_from(args)...});
    }
    template<class... Args>
    void tellmpv_async(const QByteArray &cmd, const Args &... args) {
        tellmpv_async(cmd, {qbytearray_from(args)...});
    }

    void updateMrl() {
        hasImage = startInfo.mrl.isImage();
        updateMediaName();
        emit p->mrlChanged(startInfo.mrl);
    }

    void loadfile(const Mrl &mrl, int resume, int cache, int edition) {
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
        const auto scaled = [factor] (double v) { return qBound(0., v*factor, 10.); };
        if (outline.enabled) {
            opts.add("sub-text-border-size", scaled(outline.width));
            opts.add("sub-text-border-color", outline.color.name(QColor::HexArgb).toLatin1());
        } else
            opts.add("sub-text-border-size", 0.0);
        const auto &bbox = subStyle.bbox;
        if (bbox.enabled)
            opts.add("sub-text-back-color", bbox.color.name(QColor::HexArgb).toLatin1());
        auto norm = [] (const QPointF &p) { return sqrt(p.x()*p.x() + p.y()*p.y()); };
        const auto &shadow = subStyle.shadow;
        if (shadow.enabled) {
            opts.add("sub-text-shadow-color", shadow.color.name(QColor::HexArgb).toLatin1());
            opts.add("sub-text-shadow-offset", scaled(norm(shadow.offset)));
        } else
            opts.add("sub-text-shadow-offset", 0.0);

        if (cache > 0) {
            opts.add("cache", cache);
            opts.add("cache-pause", (cacheForPlayback > 0 ? QByteArray::number(qMax<int>(1, cacheForPlayback*0.01)) : "no"));
            opts.add("cache-min", cacheForPlayback);
            opts.add("cache-seek-min", cacheForSeeking);
        } else
            opts.add("cache", "no");
        opts.add("pause", p->isPaused() || hasImage);
        opts.add("af", af(), true);
        opts.add("channels", ChannelLayoutInfo::data(layout), true);
        OptionList vo(':');
        vo.add("address", video);
        vo.add("swdec_deint", deint_swdec.toString().toLatin1());
        vo.add("hwdec_deint", deint_hwdec.toString().toLatin1());
        opts.add("vo", "null:" + vo.get(), true);
        _Debug("Load: %% (%%)", file, opts.get());
        tellmpv("loadfile", file.toLocal8Bit(), "replace", opts.get());
    }
    void loadfile(int resume) {
        if (startInfo.isValid())
            loadfile(startInfo.mrl, resume, startInfo.cache, startInfo.edition);
    }
    void loadfile() { loadfile(startInfo.resume); }
    void updateMediaName(const QString &name = QString()) {
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
        mediaInfo.setName(category % _L(": ") % (name.isEmpty() ? mrl.displayName() : name));
    }
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
    _Debug("Create audio/video plugins");
    d->audio = new AudioController(this);
    d->video = new VideoOutput(this);

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
    connect(d->video, &VideoOutput::formatChanged, this, &PlayEngine::updateVideoFormat);

    d->handle = mpv_create();
    auto verbose = qgetenv("CMPLAYER_MPV_VERBOSE").toLower();
    if (!verbose.isEmpty())
        mpv_request_log_messages(d->handle, verbose.constData());
    d->setOption("fs", "no");
    d->setOption("mouse-movements", "yes");
    d->setOption("softvol", "yes");
    d->setOption("softvol-max", "1000.0");
    d->setOption("autosub", "no");
    d->setOption("osd-level", "0");
    d->setOption("quiet", "yes");
    d->setOption("consolecontrols", "no");
    d->setOption("ao", "null,");
    d->setOption("ad-lavc-downmix", "no");
    d->setOption("title", "\"\"");

    auto overrides = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
    if (!overrides.isEmpty()) {
        const auto args = QString::fromLocal8Bit(overrides).split(QRegularExpression(R"([\s\t]+)"), QString::SkipEmptyParts);
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
            } else
                d->setOption(arg.left(index).toLatin1(), arg.mid(index+1).toLatin1());
        }
    }
    d->fatal(mpv_initialize(d->handle), "Couldn't initialize mpv.");
    _Debug("Initialized");
}

PlayEngine::~PlayEngine() {
    delete d->videoInfo;
    delete d->audioInfo;
    delete d->chapterInfo;
    delete d->audioTrackInfo;
    delete d->audio;
    delete d->video;
//    finalizeGL();
    mpv_destroy(d->handle);
    delete d;
    _Debug("Finalized");
}

const MetaData &PlayEngine::metaData() const {
    return d->metaData;
}

void PlayEngine::updateVideoFormat(VideoFormat format) {
    if (_Change(d->videoFormat, format))
        emit videoFormatChanged(d->videoFormat);
    auto output = d->videoInfo->m_output;
    output->setBps(d->videoFormat.bitrate(output->m_fps));
}

SubtitleTrackInfoObject *PlayEngine::subtitleTrackInfo() const {
    return &d->subtitleTrackInfo;
}

void PlayEngine::setSubtitleDelay(int ms) {
    if (_Change(d->subDelay, ms))
        d->setmpv_async("sub-delay", d->subDelay/1000.0);
}

void PlayEngine::setSubtitleTracks(const QStringList &tracks) {
    d->subtitleTrackInfo.set(tracks);
    emit subtitleTrackInfoChanged();
}

void PlayEngine::setCurrentSubtitleIndex(int idx) {
    d->subtitleTrackInfo.setCurrentIndex(idx);
}

ChapterInfoObject *PlayEngine::chapterInfo() const {
    return d->chapterInfo;
}

AudioTrackInfoObject *PlayEngine::audioTrackInfo() const {
    return d->audioTrackInfo;
}

QString PlayEngine::mediaName() const {
    return d->mediaName;
}

qreal PlayEngine::cache() const {
    return d->cache/100.0;
}

int PlayEngine::begin() const {
    return d->begin;
}

int PlayEngine::end() const {
    return d->begin + d->duration;
}

void PlayEngine::setImageDuration(int duration) {
    d->image.setDuration(duration);
}

int PlayEngine::duration() const {
    return d->duration;
}

int PlayEngine::currentEdition() const {
    return d->edition;
}

const EditionList &PlayEngine::editions() const {
    return d->editions;
}

const ChapterList &PlayEngine::chapters() const {
    return d->chapters;
}

const StreamList &PlayEngine::subtitleStreams() const {
    return d->streams[Stream::Subtitle];
}

VideoRendererItem *PlayEngine::videoRenderer() const {
    return d->renderer;
}

const StreamList &PlayEngine::videoStreams() const {
    return d->streams[Stream::Video];
}

int PlayEngine::audioSync() const {
    return d->audioSync;
}

const StreamList &PlayEngine::audioStreams() const {
    return d->streams[Stream::Audio];
}

PlayEngine::HardwareAcceleration PlayEngine::hwAcc() const {
    return d->hwacc;
}

void PlayEngine::run() {
    d->thread.start();
}

QThread *PlayEngine::thread() const {
    return &d->thread;
}

void PlayEngine::waitUntilTerminated() {
    if (d->thread.isRunning())
        d->thread.wait();
}

double PlayEngine::speed() const {
    return d->speed;
}

void PlayEngine::setSpeed(double speed) {
    if (_ChangeZ(d->speed, speed)) {
        d->setmpv_async("speed", speed);
        emit speedChanged(d->speed);
    }
}

void PlayEngine::setSubtitleStyle(const SubtitleStyle &style) {
    d->subStyle = style;
}

void PlayEngine::seek(int pos) {
    d->chapter = -1;
    if (d->hasImage)
        d->image.seek(pos, false);
    else
        d->tellmpv("seek", (double)pos/1000.0, 2);
}

void PlayEngine::relativeSeek(int pos) {
    if (d->hasImage)
        d->image.seek(pos, true);
    else
        d->tellmpv("seek", (double)pos/1000.0, 0);
    emit sought();
}

void PlayEngine::setClippingMethod(ClippingMethod method) {
    d->audio->setClippingMethod(method);
}

void PlayEngine::setChannelLayoutMap(const ChannelLayoutMap &map) {
    d->audio->setChannelLayoutMap(map);
}

void PlayEngine::reload() {
    d->startInfo.edition = d->edition;
    d->loadfile(d->position);
}

void PlayEngine::setChannelLayout(ChannelLayout layout) {
    if (_Change(d->layout, layout) && d->position > 0)
        reload();
}

typedef QPair<AudioDriver, const char*> AudioDriverName;
const std::array<AudioDriverName, AudioDriverInfo::size()-1> audioDriverNames = {{
    {AudioDriver::ALSA, "alsa"},
    {AudioDriver::OSS, "oss"},
    {AudioDriver::PulseAudio, "pulse"},
    {AudioDriver::CoreAudio, "coreaudio"},
    {AudioDriver::PortAudio, "portaudio"},
    {AudioDriver::JACK, "jack"},
    {AudioDriver::OpenAL, "openal"}
}};

void PlayEngine::setAudioDriver(AudioDriver driver) {
    if (_Change(d->audioDriver, driver)) {
        auto it = _FindIf(audioDriverNames, [driver] (const AudioDriverName &one) {
            return one.first == driver;
        });
        d->ao = it != audioDriverNames.end() ? it->second : "";
    }
}

QQuickItem *PlayEngine::screen() const {
    return d->renderer;
}

void PlayEngine::setMinimumCache(int playback, int seeking) {
    d->cacheForPlayback = playback;
    d->cacheForSeeking = seeking;
}

double PlayEngine::volumeNormalizer() const {
    auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
}

void PlayEngine::setHwAcc(int backend, const QList<int> &codecs) {
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

void PlayEngine::setSubtitleStreamsVisible(bool visible) {
    d->subStreamsVisible = visible;
    const auto id = currentSubtitleStream();
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id >= 0));
}

void PlayEngine::setCurrentSubtitleStream(int id) {
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id > 0));
    if (id > 0)
        d->setmpv_async("sub", id);
}

int PlayEngine::currentSubtitleStream() const {
    return d->streams[Stream::Subtitle].isEmpty() ? 0 : d->streamIds[Stream::Subtitle];
}

bool PlayEngine::addSubtitleStream(const QString &fileName, const QString &enc) {
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

void PlayEngine::removeSubtitleStream(int id) {
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

double PlayEngine::avgfps() const {
    return d->renderer->avgfps();
}

double PlayEngine::avgsync() const {
    return d->avsync;
}

void PlayEngine::setNextStartInfo(const StartInfo &startInfo) {
    d->nextInfo = startInfo;
}

void PlayEngine::stepFrame(int direction) {
    if ((m_state & (Playing | Paused)) && d->seekable)
        d->tellmpv_async(direction > 0 ? "frame_step" : "frame_back_step");
}

void PlayEngine::updateState(State state) {
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

template<typename T>
static bool _CheckSwap(T &the, T &one) { if (the != one) { the.swap(one); return true; } return false; }

void PlayEngine::customEvent(QEvent *event) {
    switch ((int)event->type()) {
    case UpdateChapterList: {
        auto chapters = _GetData<ChapterList>(event);
        if (_CheckSwap(d->chapters, chapters)) {
            d->chapterInfo->setCount(d->chapters.size());
            emit chaptersChanged(d->chapters);
            if (!d->chapters.isEmpty()) {
                Chapter prev; prev.m_id = -1; prev.m_time = _Min<int>();
                Chapter last; last.m_id = d->chapters.last().id() + 1; last.m_time = _Max<int>();
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
        auto check = [&] (Stream::Type type, void (PlayEngine::*sig)(const StreamList&)) {
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
        if (d->renderer)
            d->renderer->reset();
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
            emit requestNextStartInfo();
            if (d->nextInfo.isValid())
                state = Loading;
            remain = 0;
            break;
        case EndQuit: case EndRequest:
            break;
        default:
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
        d->tick = false;
        auto findChapterIn = [&] (int begin, int end) {
            end = qMin(end, d->chapterFakeList.size());
            begin = qMax(0, begin);
            for (int i=begin; i<end-1; ++i) {
                if (d->chapterFakeList[i].time() <= d->position && d->position < d->chapterFakeList[i+1].time())
                    return d->chapterFakeList[i].id();
            }
            return -2;
        };
        int chapter = -2;
        if (d->chapterFakeList.isEmpty())
            chapter = -2;
        else if (d->chapter < -1)
            chapter = findChapterIn(0, d->chapterFakeList.size());
        else {
            chapter = findChapterIn(d->chapter+1, d->chapterFakeList.size());
            if (chapter == -2)
                chapter = findChapterIn(0, d->chapter+1);
        }
        if (_Change(d->chapter, chapter))
            emit currentChapterChanged(d->chapter);
        break;
    } case UpdateAudioInfo: {
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
            static QVector<QString> types = { _L("vaapi"), _L("vdpau"), _L("vda") };
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

MediaInfoObject *PlayEngine::mediaInfo() const {
    return &d->mediaInfo;
}

AvInfoObject *PlayEngine::audioInfo() const {
    return d->audioInfo;
}

AvInfoObject *PlayEngine::videoInfo() const {
    return d->videoInfo;
}

void PlayEngine::setState(PlayEngine::State state) {
    _PostEvent(this, StateChange, state);
}

void PlayEngine::setCurrentChapter(int id) {
    d->setmpv_async("chapter", id);
}

void PlayEngine::setCurrentEdition(int id, int from) {
    const auto mrl = d->startInfo.mrl;
    if (id == DVDMenu && mrl.isDisc()) {
        static const char *cmds[] = {"discnav", "menu", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds), "Couldn't send 'discnav menu'.");
    } else if (0 <= id && id < d->editions.size()) {
        d->setmpv(mrl.isDisc() ? "disc-title" : "edition", id);
        seek(from);
    }
}

void PlayEngine::setVolume(int volume) {
    if (_Change(d->volume, qBound(0, volume, 100))) {
        d->setmpv_async("volume", d->mpVolume());
        emit volumeChanged(d->volume);
    }
}

bool PlayEngine::isMuted() const {
    return d->muted;
}

int PlayEngine::volume() const {
    return d->volume;
}

double PlayEngine::amp() const {
    return d->amp;
}

void PlayEngine::setAmp(double amp) {
    if (_ChangeZ(d->amp, qBound(0.0, amp, 10.0))) {
        d->setmpv_async("volume", d->mpVolume());
        emit preampChanged(d->amp);
    }
}

void PlayEngine::setMuted(bool muted) {
    if (_Change(d->muted, muted)) {
        d->setmpv_async("mute", d->muted);
        emit mutedChanged(d->muted);
    }
}

void PlayEngine::exec() {
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
        if (_Change(position, time(d->getmpv<double>("time-pos"))) && position > 0) {
            if (first) {
                duration = time(d->getmpv<double>("length"));
                _PostEvent(this, UpdateTimeRange, time(d->getmpv<double>("time-start")), duration);
                const auto array = d->getmpv<QVariant>("chapter-list").toList();
                ChapterList chapters; chapters.resize(array.size());
                for (int i=0; i<array.size(); ++i) {
                    const auto map = array[i].toMap();
                    auto &chapter = chapters[i];
                    chapter.m_id = i;
                    chapter.m_time = time(map["time"].toDouble());
                    chapter.m_name = map["title"].toString();
                    if (chapter.m_name.isEmpty())
                        chapter.m_name = _MSecToString(chapter.m_time, _L("hh:mm:ss.zzz"));
                }
                _PostEvent(this, UpdateChapterList, chapters);
                _PostEvent(this, UpdateMetaData, metaData());
                first = false;
            }
            double sync = 0;
            if (d->isSuccess(mpv_get_property(d->handle, "avsync", MPV_FORMAT_DOUBLE, &sync)) && d->renderer)
                sync = sync*1000.0 - d->renderer->delay();
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
                const auto res = mpv_get_property(d->handle, "cache", MPV_FORMAT_INT64, &newCache);
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
                Log::write(Log::Info, "[mpv/%%] %%", message->prefix, leftmsg.mid(from, to-from));
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
        } case MPV_EVENT_TRACKS_CHANGED: {
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
                if (_InRange(2, stream.m_lang.size(), 3) && _IsAlphabet(stream.m_lang))
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
                auto data = static_cast<QByteArray*>((void*)event->reply_userdata);
                _Debug("Error %%: Couldn't set property %%.", mpv_error_string(event->error), *data);
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

            const auto in = d->audio->inputFormat(), out = d->audio->outputFormat();
            audio->m_input->m_bitrate = d->getmpv<int>("audio-bitrate")*8;
            audio->m_input->m_samplerate = d->getmpv<int>("samplerate")/1000.0;
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

void PlayEngine::shutdown() {
    d->tellmpv("quit 1");
}

const StartInfo &PlayEngine::startInfo() const {
    return d->startInfo;
}

void PlayEngine::load(const StartInfo &info) {
    const bool changed = d->startInfo.mrl != info.mrl;
    d->startInfo = info;
    if (changed)
        d->updateMrl();
    if (info.isValid())
        d->loadfile();
}

int PlayEngine::time() const {
    return d->position;
}

bool PlayEngine::isSeekable() const {
    return d->seekable;
}

bool PlayEngine::hasVideo() const {
    return !d->streams[Stream::Video].isEmpty();
}

int PlayEngine::currentChapter() const {
    return d->chapter;
}

void PlayEngine::pause() {
    if (d->hasImage)
        setState(PlayEngine::Paused);
    else
        d->setmpv("pause", true);
}

void PlayEngine::unpause() {
    if (d->hasImage)
        setState(PlayEngine::Playing);
    else
        d->setmpv("pause", false);
}

Mrl PlayEngine::mrl() const {
    return d->startInfo.mrl;
}

int PlayEngine::currentAudioStream() const {
    return d->streamIds[Stream::Audio];
}

void PlayEngine::setCurrentVideoStream(int id) {
    if (d->streams[Stream::Video].contains(id))
        d->setmpv_async("video", id);
}

int PlayEngine::currentVideoStream() const {
    return hasVideo() ? d->streamIds[Stream::Video] : 0;
}

void PlayEngine::setCurrentAudioStream(int id) {
    if (d->streams[Stream::Audio].contains(id))
        d->setmpv_async("audio", id);
}

void PlayEngine::setAudioSync(int sync) {
    if (_Change(d->audioSync, sync))
        d->setmpv_async("audio-delay", sync*0.001);
}

double PlayEngine::fps() const {
    return d->fps;
}

void PlayEngine::setVideoRenderer(VideoRendererItem *renderer) {
    if (d->renderer != renderer) {
        for (auto &conn : d->rendererConnections)
            disconnect(conn);
        d->rendererConnections.clear();
        d->renderer = renderer;
        d->video->setRenderer(d->renderer);
        if (d->renderer)
            d->rendererConnections << connect(d->renderer
                , &VideoRendererItem::droppedFramesChanged, this, &PlayEngine::droppedFramesChanged);
    }
}

int PlayEngine::droppedFrames() const {
    return d->renderer ? d->renderer->droppedFrames() : 0;
}

double PlayEngine::bitrate(double fps) const {
    return d->videoFormat.bitrate(fps);
}

VideoFormat PlayEngine::videoFormat() const {
    return d->videoFormat;
}

void PlayEngine::registerObjects() {
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

void PlayEngine::setVolumeNormalizerActivated(bool on) {
    if (d->audio->isNormalizerActivated() != on) {
        d->audio->setNormalizerActivated(on);
        emit volumeNormalizerActivatedChanged(on);
    }
}

void PlayEngine::setTempoScalerActivated(bool on) {
    if (_Change(d->tempoScaler, on)) {
        d->tellmpv("af", "set", d->af());
        emit tempoScaledChanged(on);
    }
}

bool PlayEngine::isVolumeNormalizerActivated() const {
    return d->audio->isNormalizerActivated();
}

bool PlayEngine::isTempoScaled() const {
    return d->audio->isTempoScalerActivated();
}

void PlayEngine::stop() {
    d->tellmpv("stop");
}

void PlayEngine::setVolumeNormalizerOption(double length, double target, double silence, double min, double max) {
    d->audio->setNormalizerOption(length, target, silence, min, max);
}

void PlayEngine::setDeintOptions(const DeintOption &swdec, const DeintOption &hwdec) {
    d->deint_swdec = swdec;
    d->deint_hwdec = hwdec;
}

void PlayEngine::setDeintMode(DeintMode mode) {
    if (_Change(d->deint, mode))
        d->setmpv_async("deinterlace", !!(int)mode);
}

DeintMode PlayEngine::deintMode() const {
    return d->deint;
}

QString PlayEngine::stateText(State state) {
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

QString PlayEngine::stateText() const { return stateText(m_state); }

void PlayEngine::sendMouseClick(const QPointF &pos) {
    if (d->handle && d->disc) {
        if (_Change(d->mouse, pos.toPoint()))
            d->renderer->setMousePosition(d->mouse);
        static const char *cmds[] = {"discnav", "mouse", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds), "Couldn't send mouse.");
    }
}

void PlayEngine::sendMouseMove(const QPointF &pos) {
    if (d->handle && d->disc && _Change(d->mouse, pos.toPoint())) {
        d->renderer->setMousePosition(d->mouse);
        static const char *cmds[] = {"discnav", "mouse_move", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds), "Couldn't send mouse_move.");
    }
}

QList<SubtitleFileInfo> PlayEngine::subtitleFiles() const {
    return d->subtitleFiles;
}
