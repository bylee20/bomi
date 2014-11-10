#include "playengine_p.hpp"

auto reg_play_engine() -> void
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
    OptionList vo(':');
    vo.add("null:address"_b, video);
    return vo.get();
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
//    mpv_suspend(handle);
    OptionList opts;
    opts.add("audio-device"_b, audioDevice.toLatin1(), true);
    if (hwaccCodecs.isEmpty() || hwaccBackend == HwAcc::None)
        opts.add("hwdec"_b, "no"_b);
    else {
        const auto name = HwAcc::backendName(hwaccBackend);
        opts.add("hwdec"_b, name.toLatin1());
        opts.add("hwdec-codecs"_b, hwaccCodecs, true);
    }

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
    double factor = font.size;
    if (font.scale == OsdScalePolicy::Width)
        factor *= 1280;
    else if (font.scale == OsdScalePolicy::Diagonal)
        factor *= sqrt(1280*1280 + 720*720);
    else
        factor *= 720.0;
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
    _Debug("Load: %% (%%)", file, opts.get());
    tellmpv("loadfile"_b, file.toLocal8Bit(), "replace"_b, opts.get());
//    mpv_resume(handle);
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

auto translator_display_language(const QString &iso) -> QString;

SIA _IsAlphabet(ushort c) -> bool
{
    return _InRange<ushort>('a', c, 'z') || _InRange<ushort>('A', c, 'Z');
}

SIA _IsAlphabet(const QString &text) -> bool
{
    for (auto &c : text) { if (!_IsAlphabet(c.unicode())) return false; }
    return true;
}

auto PlayEngine::Data::observe() -> void
{
    observations.resize(UpdateEventEnd - UpdateEventBegin);

    observe(UpdateCacheUsed, "cache-used", cacheUsed, [=] () {
        return cacheEnabled ? getmpv<int>("cache-used") : 0;
    }, &PlayEngine::cacheUsedChanged);
    observe(UpdateCacheSize, "cache-size", cacheSize, [=] () {
        return cacheEnabled ? getmpv<int>("cache-size") : 0;
    }, &PlayEngine::cacheSizeChanged);
    observeTime(UpdateTimePos, "time-pos", position, &PlayEngine::tick);
    observeTime(UpdateTimeStart, "time-start", begin, &PlayEngine::beginChanged);
    observeTime(UpdateTimeLength, "length", duration, &PlayEngine::durationChanged);
    observeTime(UpdateAvSync, "avsync", avSync, &PlayEngine::avSyncChanged);
    observe(UpdateSeekable, "seekable", seekable, &PlayEngine::seekableChanged);
    observe(UpdateChapterList, "chapter-list", chapters, [=] () {
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
        chapterInfo->setCount(chapters.size());
        emit p->chaptersChanged(chapters);
    });
    observe(UpdateCurrentChapter, "chapter", chapter,
               &PlayEngine::currentChapterChanged);
    observe(UpdateTrackList, "track-list", [=] () {
        QVector<StreamList> streams(3);
        auto list = getmpv<QVariant>("track-list").toList();
        for (auto &var : list) {
            auto map = var.toMap();
            auto type = Stream::Unknown;
            switch (map[u"type"_q].toString().at(0).unicode()) {
            case 'v': type = Stream::Video; break;
            case 'a': type = Stream::Audio; break;
            case 's': type = Stream::Subtitle; break;
            default: continue;
            }
            Stream stream;
            stream.m_type = type;
            stream.m_albumart = map[u"albumart"_q].toBool();
            stream.m_codec = map[u"codec"_q].toString();
            stream.m_default = map[u"default"_q].toBool();
            stream.m_id = map[u"id"_q].toInt();
            stream.m_lang = map[u"lang"_q].toString();
            if (_InRange(2, stream.m_lang.size(), 3) && _IsAlphabet(stream.m_lang))
                stream.m_lang = translator_display_language(stream.m_lang);
            stream.m_title = map[u"title"_q].toString();
            stream.m_fileName = map[u"external-filename"_q].toString();
            if (!stream.m_fileName.isEmpty())
                stream.m_title = QFileInfo(stream.m_fileName).fileName();
            stream.m_selected = map[u"selected"_q].toBool();
            streams[stream.m_type].insert(stream.m_id, stream);
        }
        return streams;
    }, [=] (QEvent *event) {
        auto strms = _MoveData<QVector<StreamList>>(event);
        auto check = [&] (Stream::Type type)
        {
            if (!_Change(streams[type], strms[type]))
                return false;
            emit (p->*(streamTypeInfo[type].notifyList))(streams[type]);
            emit (p->*(streamTypeInfo[type].notifyTrack))(currentTrack(type));
            return true;
        };
        const bool hadVideo = !streams[Stream::Video].isEmpty();
        const bool hasVideo = !strms[Stream::Video].isEmpty();
        if (check(Stream::Video) && hasVideo != hadVideo)
                emit p->hasVideoChanged();
        if (check(Stream::Audio)) {
            audioTrackInfo->setCount(streams[Stream::Audio].size());
            audioTrackInfo->setCurrent(currentTrack(Stream::Audio));
        }
        check(Stream::Subtitle);
    });
    for (auto type : streamTypes)
        observeTrack(type);
    observe(UpdateMetaData, "metadata", metaData, [=] () {
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
    observe(UpdateMediaTitle, "media-title", [=] () {
        return getmpv<QString>("media-title");
    }, [=] (QEvent *event) { updateMediaName(_GetData<QString>(event)); });

    for (const auto &ob : observations) {
        if (ob.name)
            mpv_observe_property(handle, ob.event, ob.name, MPV_FORMAT_NONE);
    }
}

auto PlayEngine::Data::dispatch(mpv_event *event) -> void
{
    thread_local bool loaded = false;

    switch (event->event_id) {
    case MPV_EVENT_NONE:
        break;
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
            Log::write(Log::Info, "[mpv/%%] %%", message->prefix,
                       left.mid(from, to-from));
            from = to + 1;
        }
        left = left.mid(from);
        break;
    } case MPV_EVENT_IDLE:
        break;
    case MPV_EVENT_START_FILE:
        loaded = false;
        this->mpvMrl = this->startInfo.mrl;
        this->postState(Loading);
        _PostEvent(p, PreparePlayback);
        break;
    case MPV_EVENT_FILE_LOADED: {
        loaded = true;
        this->disc = this->mpvMrl.isDisc();
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
        disc = false;
        auto reason = [] (void *data) {
            const auto reason = static_cast<mpv_event_end_file*>(data)->reason;
            if (reason > EndUnknown)
                return EndUnknown;
            if (reason == EndOfFile && !loaded)
                return EndFailed;
            return static_cast<EndReason>(reason);
        };
        _PostEvent(p, EndPlayback, mpvMrl, reason(event->data));
        break;
    } case MPV_EVENT_PROPERTY_CHANGE:
        observation(event->reply_userdata).post();
        break;
    case MPV_EVENT_PAUSE: case MPV_EVENT_UNPAUSE: {
        const auto paused = getmpv<bool>("core-idle");
        const auto byCache = getmpv<bool>("paused-for-cache");
        postState(byCache ? Buffering : paused ? Paused : Playing);
        break;
    } case MPV_EVENT_SET_PROPERTY_REPLY:
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
    } case MPV_EVENT_AUDIO_RECONFIG: {
        auto audio = new AvInfoObject;
        audio->m_device = this->audioDevice;
        audio->m_codec = getmpv<QString>("audio-format");
        audio->m_codecDescription = getmpv<QString>("audio-codec");

        const auto in = this->audio->inputFormat();
        const auto out = this->audio->outputFormat();
        audio->m_input->m_bitrate = getmpv<int>("audio-bitrate")*8;
        audio->m_input->m_samplerate
                = getmpv<int>("audio-samplerate")/1000.0;
        audio->m_input->m_channels = in.channels();
        audio->m_input->m_bits = in.bits();
        audio->m_input->m_type = in.type();

        audio->m_output->m_bitrate = out.bitrate();
        audio->m_output->m_samplerate = out.samplerate();
        audio->m_output->m_channels = out.channels();
        audio->m_output->m_bits = out.bits();
        audio->m_output->m_type = out.type();

        _PostEvent(p, UpdateAudioInfo, audio);
        break;
    } case MPV_EVENT_VIDEO_RECONFIG: {
        auto video = new AvInfoObject;
        auto vin = getmpv<QVariant>("video-params").toMap();
        auto vout = getmpv<QVariant>("video-out-params").toMap();
        video->m_input->m_bitrate = getmpv<int>("video-bitrate")*8;
        video->m_input->m_type = vin[u"pixelformat"_q].toString();
        video->m_input->m_size.rwidth() = vin[u"w"_q].toInt();
        video->m_input->m_size.rheight() = vin[u"h"_q].toInt();
        video->m_input->m_fps = getmpv<double>("fps");
        video->m_output->m_type = vout[u"pixelformat"_q].toString();
        video->m_output->m_size.rwidth() = vout[u"dw"_q].toInt();
        video->m_output->m_size.rheight() = vout[u"dh"_q].toInt();
        video->m_output->m_fps = getmpv<double>("fps");
        video->m_codecDescription = getmpv<QString>("video-codec");
        video->m_codec = getmpv<QString>("video-format");
        video->moveToThread(qApp->instance()->thread());
        _PostEvent(p, UpdateVideoInfo, video);
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
    if (UpdateEventBegin <= type && type < UpdateEventEnd) {
        this->observation(event->type()).handle(event);
        return;
    }
    switch ((int)event->type()) {
     case StateChange:
        p->updateState(_GetData<PlayEngine::State>(event));
        break;
    case PreparePlayback: {
        this->subtitleFiles.clear();
        break;
    } case StartPlayback: {
        this->renderer->resetTimings();
        _TakeData(event, this->editions);
        emit p->audioChanged();
        int title = -1;
        for (auto &item : this->editions) {
            if (item.isSelected())
                title = item.id();
        }
        this->edition = title;
        emit p->editionsChanged(this->editions);
        if (!getmpv<bool>("pause"))
            p->updateState(Playing);
        emit p->started(startInfo.mrl);
        break;
    } case EndPlayback: {
        Mrl mrl; EndReason reason; _TakeData(event, mrl, reason);
        int remain = (this->duration + this->begin) - this->position;
        nextInfo = StartInfo();
        auto state = Stopped;
        switch (reason) {
        case EndOfFile:
            _Info("Playback reached end-of-file");
            emit p->requestNextStartInfo();
            if (nextInfo.isValid())
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
        p->updateState(state);
        if (state != Error && !mrl.isEmpty()) {
            FinishInfo info;
            info.mrl = mrl;
            info.position = this->position;
            info.remain = remain;
            for (auto type : streamTypes)
                info.streamIds[type] = this->currentTrack(type);
//            qDebug() << this->lastStreamIds << this->streamIds;
            emit p->finished(info);
        }
        if (nextInfo.isValid())
            p->load(nextInfo);
        break;
    }case UpdateAudioInfo: {
        delete this->audioInfo;
        this->audioInfo = _GetData<AvInfoObject*>(event);
        emit p->audioChanged();
        break;
    } case UpdateVideoInfo: {
        delete this->videoInfo;
        this->videoInfo = _GetData<AvInfoObject*>(event);
        auto output = this->videoInfo->m_output;
        output->m_bitrate = this->videoFormat.bitrate(output->m_fps);
        auto hwacc = [&] () {
            auto codec = HwAcc::codecId(this->videoInfo->codec().toLatin1());
            if (!HwAcc::supports(this->hwaccBackend, codec))
                return HardwareAcceleration::Unavailable;
            static QVector<QString> types = {
                u"vaapi"_q, u"vdpau"_q, u"vda"_q
            };
            if (types.contains(output->m_type))
                return HardwareAcceleration::Activated;
            if (this->hwaccCodecs.contains(this->videoInfo->codec().toLatin1()))
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
        if (_Change(this->hwacc, hwacc()))
            emit p->hwaccChanged();
        this->videoInfo->m_hwacc = hwtxt(this->hwacc);
        emit p->videoChanged();
        if (_Change(this->fps, output->m_fps))
            emit p->fpsChanged(this->fps);
        break;
    } case NotifySeek:
        emit p->sought();
        break;
    case UpdateMetaData: {
        this->metaData = _GetData<MetaData>(event);
        emit p->metaDataChanged();
        break;
    } default:
        break;
    }
}
