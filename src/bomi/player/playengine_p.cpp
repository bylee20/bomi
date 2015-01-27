#include "playengine_p.hpp"

template<class T>
SIA findEnum(const QString &mpv) -> T
{
    auto items = EnumInfo<T>::items();
    for (auto item : items) {
        if (mpv == item.data.property)
            return item.value;
    }
    return EnumInfo<T>::default_();
}

auto reg_play_engine() -> void
{
    qRegisterMetaType<PlayEngine::State>("State");
    qRegisterMetaType<Mrl>("Mrl");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<StreamList>("StreamList");
    qRegisterMetaType<AudioFormat>("AudioFormat");
    qmlRegisterType<ChapterInfoObject>();
    qmlRegisterType<VideoInfoObject>();
    qmlRegisterType<AvTrackInfoObject>();
    qmlRegisterType<VideoFormatInfoObject>();
    qmlRegisterType<VideoHwAccInfoObject>();
    qmlRegisterType<AudioFormatInfoObject>();
    qmlRegisterType<AudioInfoObject>();
    qmlRegisterType<CodecInfoObject>();
    qmlRegisterType<MediaInfoObject>();
    qmlRegisterType<SubtitleInfoObject>();
    qmlRegisterType<PlayEngine>("bomi", 1, 0, "Engine");
}

class OptionList {
public:
    OptionList(char join = ',')
        : m_join(join) { }
    auto add(const QByteArray &key, const QByteArray &value,
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
    auto add(const QByteArray &key, void *value) -> void
        { add(key, address_cast<QByteArray>(value)); }
    auto add(const QByteArray &key, double value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const QByteArray &key, int value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const QByteArray &key, bool value) -> void
        { add(key, value ? "yes"_b : "no"_b); }
    auto get() const -> const QByteArray& { return m_data; }
    auto data() const -> const char* { return m_data.data(); }
private:
    QByteArray m_data;
    char m_join;
};

PlayEngine::Data::Data(PlayEngine *engine)
    : p(engine) { }

auto PlayEngine::Data::af() const -> QByteArray
{
    OptionList af(':');
    af.add("dummy:address"_b, audio);
    af.add("use_scaler"_b, (int)tempoScaler);
    af.add("layout"_b, (int)layout);
    return af.get();
}

auto PlayEngine::Data::vf() const -> QByteArray
{
    OptionList vf(':');
    vf.add("noformat:address"_b, filter);
    vf.add("swdec_deint"_b, deint_swdec.toString().toLatin1());
    vf.add("hwdec_deint"_b, deint_hwdec.toString().toLatin1());
    return vf.get();
}

auto PlayEngine::Data::vo() const -> QByteArray
{
    return "opengl-cb:"
            + videoSubOptions();
}

auto PlayEngine::Data::videoSubOptions() const -> QByteArray
{
    static const QByteArray shader =
            "const mat4 c_matrix = mat4(__C_MATRIX__); "
            "vec4 custom_shader(vec4 color) { return c_matrix * color; }";
    auto customShader = [&] (const QMatrix4x4 &c_matrix) -> QByteArray {
        QByteArray mat;
        for (int c = 0; c < 4; ++c) {
            mat += "vec4(";
            for (int r = 0; r < 4; ++r) {
                mat += QByteArray::number(c_matrix(r, c), 'e');
                mat += ',';
            }
            mat[mat.size()-1] = ')';
            mat += ',';
        }
        mat.chop(1);
        auto cs = shader;
        cs.replace("__C_MATRIX__", mat);
        return '%' + QByteArray::number(cs.length()) + '%' + cs;
    };

    OptionList opts(':');

    auto addScale = [&] (const char *opt, Interpolator scale) {
        int lanczos = 0;
        switch (scale) {
        case Interpolator::Lanczos2:
            lanczos = 2;
            break;
        case Interpolator::Lanczos3:
            lanczos = 3;
            break;
        case Interpolator::Lanczos4:
            lanczos = 4;
            break;
        default:
            break;
        }
        if (lanczos) {
            opts.add(opt, "lanczos"_b);
            opts.add("scale-radius", lanczos);
        } else
            opts.add(opt, _EnumData(scale));
    };
    addScale("scale", lscale);
    addScale("cscale", cscale);
    opts.add("dither-depth", "auto"_b);
    opts.add("dither", _EnumData(dithering));
    if (filter->isSkipping())
        opts.add("frame-queue-size", 1);
    else
        opts.add("frame-queue-size", 3);
    opts.add("frame-drop-mode", "clear"_b);
    opts.add("fancy-downscaling", hqDownscaling);
    opts.add("sigmoid-upscaling", hqUpscaling);
    opts.add("custom-shader", customShader(c_matrix));

    return opts.get();
}

auto PlayEngine::Data::updateColorMatrix() -> void
{

    c_matrix = QMatrix4x4();
    if (videoEffects & VideoEffect::Invert)
        c_matrix = QMatrix4x4(-1, 0, 0, 1,
                              0, -1, 0, 1,
                              0, 0, -1, 1,
                              0, 0,  0, 1);
    auto eq = videoEq;
    if (videoEffects & VideoEffect::Gray)
        eq.setSaturation(-100);
    c_matrix *= eq.matrix();
    if (videoEffects & VideoEffect::Remap) {
        const float a = 255.0 / (235.0 - 16.0);
        const float b = -16.0 / 255.0 * a;
        c_matrix *= QMatrix4x4(a, 0, 0, b,
                               0, a, 0, b,
                               0, 0, a, b,
                               0, 0, 0, 1);
    }
}

auto PlayEngine::Data::updateVideoSubOptions() -> void
{
    tellmpv_async("vo_cmdline", videoSubOptions());
}

auto PlayEngine::Data::tellmpv(const QByteArray &cmd) -> void
{
    if (handle)
        check(mpv_command_string(handle, cmd.constData()),
              "Cannaot execute: %%", cmd);
}

auto PlayEngine::Data::tellmpv_async(const QByteArray &cmd,
                   std::initializer_list<QByteArray> &&list) -> void
{
    QVector<const char*> args(list.size()+2, nullptr);
    auto it = args.begin();
    *it++ = cmd.constData();
    for (auto &one : list)
        *it++ = one.constData();
    if (handle)
        check(mpv_command_async(handle, 0, args.data()), "Cannot execute: %%", cmd);
}

auto PlayEngine::Data::tellmpv(const QByteArray &cmd,
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

auto PlayEngine::Data::loadfile(const Mrl &mrl, int resume, int cache,
                                int edition) -> void
{
    QString file = mrl.isLocalFile() ? mrl.toLocalFile() : mrl.toString();
    if (file.isEmpty())
        return;
    OptionList opts;
    opts.add("audio-device"_b, audioDevice.toLatin1(), true);
    if (useHwAcc)
        opts.add("hwdec-codecs"_b, hwCodecs, true);
    else
        opts.add("hwdec-codecs"_b, "", true);

    if (mrl.isDisc()) {
        file = mrl.titleMrl(edition >= 0 ? edition : -1).toString();
        initSeek = resume;
    } else {
        if (edition >= 0)
            opts.add("edition"_b, edition);
        if (resume > 0)
            opts.add("start"_b, resume/1000.0);
        initSeek = -1;
    }
    opts.add("deinterlace"_b, deint != DeintMode::None);
    opts.add("volume"_b, mpVolume());
    opts.add("mute"_b, muted);
    opts.add("audio-delay"_b, audioSync/1000.0);
    opts.add("sub-delay"_b, subDelay/1000.0);

    const auto &font = subStyle.font;
    opts.add("sub-text-color"_b, font.color.name(QColor::HexArgb).toLatin1());
    QStringList fontStyles;
    if (font.bold())
        fontStyles.append(u"Bold"_q);
    if (font.italic())
        fontStyles.append(u"Italic"_q);
    QString family = font.family();
    if (!fontStyles.isEmpty())
        family += ":style="_a % fontStyles.join(' '_q);
    const double factor = font.size * 720.0;
    opts.add("sub-text-font"_b, family.toLatin1(), true);
    opts.add("sub-text-font-size"_b, factor);
    const auto &outline = subStyle.outline;
    const auto scaled = [factor] (double v)
        { return qBound(0., v*factor, 10.); };
    const auto color = [] (const QColor &color)
        { return color.name(QColor::HexArgb).toLatin1(); };
    if (outline.enabled) {
        opts.add("sub-text-border-size"_b, scaled(outline.width));
        opts.add("sub-text-border-color"_b, color(outline.color));
    } else
        opts.add("sub-text-border-size"_b, 0.0);
    const auto &bbox = subStyle.bbox;
    if (bbox.enabled)
        opts.add("sub-text-back-color"_b, color(bbox.color));
    auto norm = [] (const QPointF &p)
        { return sqrt(p.x()*p.x() + p.y()*p.y()); };
    const auto &shadow = subStyle.shadow;
    if (shadow.enabled) {
        opts.add("sub-text-shadow-color"_b, color(shadow.color));
        opts.add("sub-text-shadow-offset"_b, scaled(norm(shadow.offset)));
    } else
        opts.add("sub-text-shadow-offset"_b, 0.0);

    if ((cacheEnabled = (cache > 0))) {
        opts.add("cache"_b, cache);
        opts.add("cache-initial"_b, int(cache*cacheForPlayback));
        opts.add("cache-seek-min"_b, int(cache*cacheForSeeking));
    } else
        opts.add("cache"_b, "no"_b);
    opts.add("pause"_b, p->isPaused() || hasImage);
    opts.add("audio-channels"_b, ChannelLayoutInfo::data(layout), true);

    opts.add("af"_b, af(), true);
    opts.add("vf"_b, vf(), true);

    opts.add("colormatrix"_b, _EnumData(colorSpace).option);
    opts.add("colormatrix-input-range", _EnumData(colorRange).option);
    opts.add("vo"_b, vo(), true);
    _Debug("Load: %% (%%)", file, opts.get());
    tellmpv("loadfile"_b, file.toLocal8Bit(), "replace"_b, opts.get());
}

auto PlayEngine::Data::updateMrl() -> void
{
    hasImage = startInfo.mrl.isImage();
    updateMediaName();
    emit p->mrlChanged(startInfo.mrl);
}

auto PlayEngine::Data::loadfile(int resume) -> void
{
    if (startInfo.isValid())
        loadfile(startInfo.mrl, resume, startInfo.cache, startInfo.edition);
}

auto PlayEngine::Data::updateMediaName(const QString &name) -> void
{
    mediaName = name;
    QString category;
    auto mrl = p->mrl();
    if (mrl.isLocalFile())
        category = tr("File");
    else if (mrl.isDvd())
        category = u"DVD"_q;
    else if (mrl.isBluray())
        category = tr("Blu-ray");
    else
        category = u"URL"_q;
    const QString display = name.isEmpty() ? mrl.displayName() : name;
    mediaInfo.setName(category % ": "_a % display);
}

auto PlayEngine::Data::setStreamList(StreamType type, StreamList &&list) -> bool
{
    auto &info = streamTypeInfo[type];
    auto &data = streams[type];
    int id = data.reserved;
    if (id < 0) {
        const auto &pri = streams[type].priority;
        auto findLang = [&] () {
            for (auto &p : pri) {
                const QRegEx rx(p);
                for (auto &str : list) {
                    auto m = rx.match(str.language());
                    if (m.hasMatch())
                        return str.id();
                }
            }
            return -1;
        };
        if (!pri.isEmpty())
            id = findLang();
    }
    if (id >= 0) {
        for (auto &str : list)
            str.m_selected = str.m_id == id;
        setmpv(info.mpvName, id);
    }
    data.reserved = -1;
    if (streams[type].tracks == list)
        return false;
    streams[type].tracks = std::move(list);
    emit (p->*(info.notifyList))(streams[type].tracks);
    emit (p->*(info.notifyTrack))(currentTrack(type));
    return true;
}

auto PlayEngine::Data::hook() -> void
{
    hook("on_load", [=] () {
        auto file = getmpv<QString>("stream-open-filename");
        if (!file.startsWith("http://"_a) && !file.startsWith("https://"_a))
            return;
        file = QUrl(file).toString(QUrl::FullyEncoded);
        if (yle && yle->supports(file)) {
            if (!yle->run(file))
                return;
            setmpv("stream-open-filename", yle->url().toLocal8Bit());
        } else if (youtube && youtube->run(file)) {
            setmpv("options/cookies", true);
            setmpv("options/cookies-file", youtube->cookies().toLocal8Bit());
            setmpv("options/user-agent", youtube->userAgent().toLocal8Bit());
            setmpv("stream-open-filename", youtube->url().toLocal8Bit());
        } else
            setmpv("stream-open-filename", file.toLocal8Bit());
    });
}

auto PlayEngine::Data::observe() -> void
{
    observeType<bool>("pause", [=] (bool p) {
        if (p)
            post(Paused);
        else if (!getmpv<bool>("idle"))
            post(Playing);
    });
    observeType<bool>("core-idle", [=] (bool i) { if (!i) post(Playing); });
    observeType<bool>("paused-for-cache", [=] (bool b) { post(Buffering, b); });
    observeType<bool>("seeking", [=] (bool s) { post(Seeking, s); });

    observe("cache-used", cacheUsed, [=] () {
        return cacheEnabled ? getmpv<int>("cache-used") : 0;
    }, &PlayEngine::cacheUsedChanged);
    observe("cache-size", cacheSize, [=] () {
        return cacheEnabled ? getmpv<int>("cache-size") : 0;
    }, &PlayEngine::cacheSizeChanged);
    observeTime("time-pos", position, &PlayEngine::tick);
    observeTime("time-start", begin, &PlayEngine::beginChanged);
    observeTime("length", duration, &PlayEngine::durationChanged);
    observeTime("avsync", avSync, &PlayEngine::avSyncChanged);
    observe("seekable", seekable, &PlayEngine::seekableChanged);
    observe("chapter-list", chapters, [=] () {
        const auto array = getmpv<QVariant>("chapter-list").toList();
        ChapterList chapters; chapters.resize(array.size());
        for (int i=0; i<array.size(); ++i) {
            const auto map = array[i].toMap();
            auto &chapter = chapters[i];
            chapter.m_id = i;
            chapter.m_time = s2ms(map[u"time"_q].toDouble());
            chapter.m_name = map[u"title"_q].toString();
            if (chapter.m_name.isEmpty())
                chapter.m_name = _MSecToString(chapter.m_time, u"hh:mm:ss.zzz"_q);
        }
        return chapters;
    }, [=] () {
        while (chapterInfoList.size() > chapters.size())
            delete chapterInfoList.takeLast();
        while (chapterInfoList.size() < chapters.size())
            chapterInfoList.push_back(new ChapterInfoObject);
        for (int i = 0; i < chapterInfoList.size(); ++i) {
            auto info = chapterInfoList[i];
            info->m_id = chapters[i].id();
            info->m_time = chapters[i].time();
            info->m_rate = p->rate(chapters[i].time());
            info->m_name = chapters[i].name();
            auto sr = [=] () { info->setRate(p->rate(info->time())); };
            connect(p, &PlayEngine::durationChanged, info, sr);
            connect(p, &PlayEngine::beginChanged, info, sr);
        }
        emit p->chaptersChanged(chapters);
    });
    observe("chapter", chapter, &PlayEngine::currentChapterChanged);
    observe("track-list", [=] () {
        QVector<StreamList> streams(3);
        auto list = getmpv<QVariant>("track-list").toList();
        for (auto &var : list) {
            const auto track = StreamTrack::fromMpvData(var);
            streams[track.type()].insert(track.id(), track);
        }
        return streams;
    }, [=] (QEvent *event) {
        auto strms = _MoveData<QVector<StreamList>>(event);
        for (auto type : streamTypes)
            setStreamList(type, std::move(strms[type]));
    });
    for (auto type : streamTypes)
        observeTrack(type);
    observe("metadata", metaData, [=] () {
        const auto list = getmpv<QVariant>("metadata").toList();
        MetaData metaData;
        for (int i=0; i+1<list.size(); i+=2) {
            const auto key = list[i].toString();
            const auto value = list[i+1].toString();
            if (key == "title"_a)
                metaData.m_title = value;
            else if (key == "artist"_a)
                metaData.m_artist = value;
            else if (key == "album"_a)
                metaData.m_album = value;
            else if (key == "genre"_a)
                metaData.m_genre = value;
            else if (key == "date"_a)
                metaData.m_date = value;
        }
        metaData.m_mrl = mpvMrl;
        metaData.m_duration = s2ms(getmpv<double>("length"));
        return metaData;
    }, &PlayEngine::metaDataChanged);
    observeType<QString>("media-title", [=] (QString &&t)
        { updateMediaName(mpvMrl.isYouTube() ? mpvMrl.toString() : t); });

    observeType<QString>("video-codec", [=] (QString &&c) { videoInfo.codec()->parse(c); });
    observeType<double>("fps", [=] (double fps) {
        videoInfo.input()->setFps(fps);
        videoInfo.output()->setFps(fps);
    });
    observeType<int>("width", [=] (int w) {
        auto input = videoInfo.input();
        input->setWidth(w);
        input->setBppSize(input->size());
    });
    observeType<int>("height", [=] (int h) {
        auto input = videoInfo.input();
        input->setHeight(h);
        input->setBppSize(input->size());
    });
    observeType<int>("video-bitrate", [=] (int bps) { videoInfo.input()->setBitrate(bps); });
    observeType<QString>("video-format", [=] (QString &&f) { videoInfo.input()->setType(f); });
    QRegularExpression rx(uR"(Video decoder: ([^\n]*))"_q);
    auto decoderOutput = [=] (const char *name) -> QString {
        auto m = rx.match(getmpvosd(name));
        if (!m.hasMatch() || m.capturedRef(1) == "unknown"_a)
            return u"Autoselect"_q;
        return m.captured(1);
    };

    auto setParams = [] (VideoFormatInfoObject *info, const QVariantMap &p,
                         const QString &wkey, const QString &hkey) {
        const auto type = p[u"pixelformat"_q].toString();
        const auto w = p[wkey].toInt(), h = p[hkey].toInt();
        info->setType(type);
        info->setSize({w, h});
        info->setBppSize({p[u"w"_q].toInt(), p[u"h"_q].toInt()},
                         p[u"average-bpp"_q].toInt());
        info->setDepth(p[u"plane-depth"_q].toInt());
    };
    observeType<QVariant>("video-params", [=] (QVariant &&var) {
        const auto params = var.toMap();
        auto info = videoInfo.output();
        setParams(info, params, u"w"_q, u"h"_q);
        info->setRange(findEnum<ColorRange>(decoderOutput("colormatrix-input-range")));
        info->setSpace(findEnum<ColorSpace>(decoderOutput("colormatrix")));
        auto hwState = [&] () {
            if (!useHwAcc)
                return Deactivated;
            static QVector<QString> types = { u"vaapi"_q, u"vdpau"_q, u"vda"_q };
            const auto codec = videoInfo.codec()->type();
            if (!HwAcc::supports(codec))
                return Unavailable;
            if (types.contains(info->type().toLower()))
                return Activated;
            if (!hwCodecs.contains(codec.toLatin1()))
                return Unavailable;
            return Deactivated;
        };
        auto hwacc = videoInfo.hwacc();
        hwacc->setState(hwState());
        const auto hwdec = getmpv<QString>("hwdec");
        hwacc->setDriver(hwdec == "no"_a ? QString() : hwdec);
    });
    observeType<QVariant>("video-out-params", [=] (QVariant &&var) {
        const auto params = var.toMap();
        auto info = videoInfo.renderer();
        setParams(info, params, u"dw"_q, u"dh"_q);
        info->setRange(findEnum<ColorRange>(params[u"colorlevels"_q].toString()));
        info->setSpace(findEnum<ColorSpace>(params[u"colormatrix"_q].toString()));
    });

    observeType<QString>("audio-codec", [=] (QString &&c) { audioInfo.codec()->parse(c); });
    observeType<QString>("audio-format", [=] (QString &&f) { audioInfo.input()->setType(f); });
    observeType<int>("audio-bitrate", [=] (int bps) { audioInfo.input()->setBitrate(bps); });
    observeType<int>("audio-samplerate", [=] (int s) { audioInfo.input()->setSampleRate(s, false); });
    observeType<int>("audio-channels", [=] (int n)
        { audioInfo.input()->setChannels(QString::number(n) % "ch"_a, n); });
    observeType<QString>("audio-device", [=] (QString &&d) { audioInfo.setDevice(d); });

    for (const auto &ob : observations) {
        if (ob.name)
            mpv_observe_property(handle, ob.event, ob.name, MPV_FORMAT_NONE);
    }
}

auto PlayEngine::Data::dispatch(mpv_event *event) -> void
{
    switch (event->event_id) {
    case MPV_EVENT_LOG_MESSAGE: {
        thread_local QMap<QByteArray, QByteArray> leftmsg;
        auto message = static_cast<mpv_event_log_message*>(event->data);
        const QByteArray prefix(message->prefix);
        auto &left = leftmsg[prefix];
        left.append(message->text);
        int from = 0;
        for (;;) {
            auto to = left.indexOf('\n', from);
            if (to < 0)
                break;
            log(prefix, left.mid(from, to-from));
            from = to + 1;
        }
        left = left.mid(from);
        break;
    } case MPV_EVENT_CLIENT_MESSAGE: {
        auto message = static_cast<mpv_event_client_message*>(event->data);
        if (message->num_args < 1)
            break;
        if (message->args[0] == "hook_run"_b && message->num_args == 3) {
            QByteArray when(message->args[2]);
            Q_ASSERT(hooks.contains(when));
            hooks[when]();
            tellmpv("hook_ack", when);
        }
        break;
    } case MPV_EVENT_IDLE:
        if (mpvState != MpvLoading)
            mpvState = MpvStopped;
        break;
    case MPV_EVENT_START_FILE:
        mpvState = MpvLoading;
        mpvMrl = startInfo.mrl;
        _PostEvent(p, PreparePlayback);
        post(getmpv<bool>("pause") ? Paused : Playing);
        post(Loading, true);
        break;
    case MPV_EVENT_FILE_LOADED: {
        post(getmpv<bool>("pause") ? Paused : Playing);
        post(Loading, false);
        updateVideoSubOptions();
        mpvState = MpvRunning;
        this->disc = mpvMrl.isDisc();
        if (this->initSeek > 0) {
            this->tellmpv("seek", this->initSeek, 2);
            this->initSeek = -1;
        }
        const char *listprop = this->disc ? "disc-titles" : "editions";
        const char *itemprop = this->disc ? "disc-title"  : "edition";
        EditionList editions;
        auto add = [&] (int id) -> Edition& {
            auto &title = editions[id];
            title.m_id = id;
            title.m_name = tr("Title %1").arg(id+1);
            return title;
        };
        const int list = getmpv<int>(listprop);
        editions.resize(list);
        for (int i=0; i<list; ++i)
            add(i);
        if (list > 0) {
            const int item = getmpv<int>(itemprop);
            if (0 <= item && item < list)
                editions[item].m_selected = true;
        }
        _PostEvent(p, StartPlayback, editions);
        break;
    } case MPV_EVENT_END_FILE: {
        post(Loading, false);
        disc = false;
        auto reason = static_cast<mpv_event_end_file*>(event->data)->reason;
        if (reason == MPV_END_FILE_REASON_EOF && mpvState != MpvRunning)
            reason = MPV_END_FILE_REASON_ERROR;
        mpvState = MpvStopped;
        _PostEvent(p, EndPlayback, mpvMrl, reason);
        break;
    } case MPV_EVENT_PROPERTY_CHANGE:
        observation(event->reply_userdata).post();
        break;
    case MPV_EVENT_SET_PROPERTY_REPLY:
        if (!isSuccess(event->error)) {
            auto ptr = reinterpret_cast<void*>(event->reply_userdata);
            auto data = static_cast<QByteArray*>(ptr);
            _Debug("Error %%: Couldn't set property %%.",
                   mpv_error_string(event->error), *data);
            delete data;
        }
        break;
    case MPV_EVENT_GET_PROPERTY_REPLY: {
        break;
    } case MPV_EVENT_SHUTDOWN:
        quit = true;
        break;
    case MPV_EVENT_PLAYBACK_RESTART:
        _PostEvent(p, NotifySeek);
        break;
    default:
        break;
    }

}

auto PlayEngine::Data::process(QEvent *event) -> void
{
    const int type = event->type();
    if (UpdateEventBegin <= type && type < updateEventMax) {
        observation(event->type()).handle(event);
        return;
    }
    switch ((int)event->type()) {
     case StateChange:
        updateState(_GetData<PlayEngine::State>(event));
        break;
    case WaitingChange: {
        bool set = false; Waitings waitings = NoWaiting;
        _TakeData(event, waitings, set);
        setWaitings(waitings, set);
        break;
    } case PreparePlayback: {
        this->subtitleFiles.clear();
        break;
    } case StartPlayback: {
        clearTimings();
        _TakeData(event, this->editions);
        int title = -1;
        for (auto &item : this->editions) {
            if (item.isSelected())
                title = item.id();
        }
        this->edition = title;
        emit p->editionsChanged(this->editions);
        emit p->started(startInfo.mrl, startInfo.reloaded);
        startInfo.reloaded = false;
        break;
    } case EndPlayback: {
        Mrl mrl; int reason; _TakeData(event, mrl, reason);
        int remain = (this->duration + this->begin) - this->position;
        nextInfo = StartInfo();
        auto state = Stopped;
        switch (reason) {
        case MPV_END_FILE_REASON_EOF:
            _Info("Playback reached end-of-file");
            emit p->requestNextStartInfo();
            if (nextInfo.isValid())
                mpvState = MpvLoading;
            remain = 0;
            break;
        case MPV_END_FILE_REASON_QUIT:
        case MPV_END_FILE_REASON_STOP:
            _Info("Playback has been terminated by request");
            break;
        default:
            _Info("Playback has been terminated by error(s)");
            state = Error;
            startInfo.reloaded = false;
        }
        updateState(state);
        if (state != Error && !mrl.isEmpty()) {
            FinishInfo info;
            info.mrl = mrl;
            info.position = this->position;
            info.remain = remain;
            for (auto type : streamTypes)
                info.streamIds[type] = this->currentTrack(type);
            emit p->finished(info);
        }
//        updateWaiting();
        if (nextInfo.isValid())
            p->load(nextInfo);
        break;
    } case NotifySeek:
        emit p->sought();
        break;
    default:
        break;
    }
}

auto PlayEngine::Data::log(const QByteArray &prefix,
                           const QByteArray &text) -> void
{
    Log::write(Log::Info, "[mpv/%%] %%", prefix, text);
    if (text.startsWith("AO: [")) {
        constexpr int from = 5;
        const int to = text.indexOf(']');
        const auto driver = QString::fromLatin1(text.mid(from, to - from));
        QMetaObject::invokeMethod(&audioInfo, "setDriver",
                                  Qt::QueuedConnection, Q_ARG(QString, driver));
    }
}

auto PlayEngine::Data::takeSnapshot() -> void
{
    p->clearSnapshots();
    const auto size = displaySize();
    if (size.isEmpty()) {
        emit p->snapshotTaken();
        return;
    }
    OpenGLFramebufferObject fbo(size);
    auto take = [&](bool withOsd) -> QImage {
        QImage image;
        if (withOsd && !p->subtitleStreams().isEmpty()) {
            const auto was = getmpv<bool>("sub-visibility");
            if (was != withOsd)
                setmpv("sub-visibility", withOsd);
            render(&fbo);
            if (was != withOsd)
                setmpv("sub-visibility", was);
            return fbo.texture().toImage();
        }
        if (!ssNoOsd.isNull())
            return ssNoOsd;
        render(&fbo);
        return fbo.texture().toImage();
    };
    if (snapshot & VideoOnly)
        ssNoOsd = take(false);
    if (snapshot & VideoWidthOsd)
        ssWithOsd = take(true);
    emit p->snapshotTaken();
}

auto PlayEngine::Data::renderVideoFrame(OpenGLFramebufferObject *fbo) -> void
{
    const int delay = render(fbo);
    fpsMeasure.push(++drawnFrames);
    videoInfo.setDelayedFrames(delay);
    videoInfo.setDroppedFrames(getmpv<int64_t>("vo-drop-frame-count"));

    _Trace("PlayEngine::Data::renderVideoFrame(): "
           "render queued frame(%%), avgfps: %%",
           fbo->size(), videoInfo.renderer()->fps());

    if (snapshot) {
        this->takeSnapshot();
        snapshot = NoSnapshot;
    }
}
