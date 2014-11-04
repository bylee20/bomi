#include "playengine.hpp"
#include "playengine_p.hpp"

auto translator_display_language(const QString &iso) -> QString;

PlayEngine::PlayEngine()
: d(new Data(this)) {
    _Debug("Create audio/video plugins");
    d->audio = new AudioController(this);
    d->video = new VideoOutput(this);
    d->filter = new VideoFilter;

    d->chapterInfo = new ChapterInfoObject(this, this);
    d->audioTrackInfo = new AudioTrackInfoObject(this, this);
    d->updateMediaName();

    _Debug("Make registrations and connections");

    connect(d->video, &VideoOutput::formatChanged,
            this, &PlayEngine::updateVideoFormat);
    connect(d->video, &VideoOutput::droppedFramesChanged,
            this, &PlayEngine::droppedFramesChanged);

    d->handle = mpv_create();
    auto verbose = qgetenv("CMPLAYER_MPV_VERBOSE").toLower();
    if (!verbose.isEmpty())
        mpv_request_log_messages(d->handle, verbose.constData());

    auto setOption = [this] (const char *name, const char *data) {
        const auto err = mpv_set_option_string(d->handle, name, data);
        d->fatal(err, "Couldn't set option %%=%%.", name, data);
    };
    setOption("fs", "no");
    setOption("input-cursor", "yes");
    setOption("softvol", "yes");
    setOption("softvol-max", "1000.0");
    setOption("sub-auto", "no");
    setOption("osd-level", "0");
    setOption("quiet", "yes");
    setOption("input-terminal", "no");
    setOption("ad-lavc-downmix", "no");
    setOption("title", "\"\"");
    setOption("vo", d->vo().constData());
    setOption("fixed-vo", "yes");

    auto overrides = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
    if (!overrides.isEmpty()) {
        const auto opts = QString::fromLocal8Bit(overrides);
        const auto args = opts.split(QRegEx(uR"([\s\t]+)"_q),
                                     QString::SkipEmptyParts);
        for (int i=0; i<args.size(); ++i) {
            if (!args[i].startsWith("--"_a)) {
                _Error("Cannot parse option %%.", args[i]);
                continue;
            }
            const auto arg = args[i].midRef(2);
            const int index = arg.indexOf('='_q);
            if (index < 0) {
                if (arg.startsWith("no-"_a))
                    setOption(arg.mid(3).toLatin1(), "no");
                else
                    setOption(arg.toLatin1(), "yes");
            } else {
                const auto key = arg.left(index).toLatin1();
                const auto value = arg.mid(index+1).toLatin1();
                setOption(key, value);
            }
        }
    }
    d->fatal(mpv_initialize(d->handle), "Couldn't initialize mpv.");
    _Debug("Initialized");
    d->initialized = true;
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
    d->initialized = false;
    mpv_terminate_destroy(d->handle);
    delete d;
    _Debug("Finalized");
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

auto PlayEngine::videoRenderer() const -> VideoRenderer*
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

SIA _ChangeZ(double &the, double one) -> bool
{
    if (qFuzzyCompare(one, 1.0))
        one = 1.0;
    if (!qFuzzyCompare(the, one)) {
        the = one;
        return true;
    }
    return false;
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
    if (!d->hasImage)
        d->tellmpv("seek", (double)pos/1000.0, 2);
}

auto PlayEngine::relativeSeek(int pos) -> void
{
    if (!d->hasImage) {
        d->tellmpv("seek", (double)pos/1000.0, 0);
        emit sought();
    }
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

auto PlayEngine::setAudioDevice(const QString &device) -> void
{
    d->audioDevice = device;
//    d->ao = device.toLocal8Bit();
//    if (_Change(d->audioDriver, driver)) {
//        auto it = std::find_if(
//            audioDriverNames.begin(), audioDriverNames.end(),
//            [driver] (const AudioDriverName &one) {return one.first == driver;}
//        );
//        d->ao = it != audioDriverNames.end() ? it->second : "";
//    }
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

auto PlayEngine::setHwAcc(int backend, const QVector<int> &codecs) -> void
{
    d->hwaccCodecs = _ToStringList(codecs, [] (int id) {
        const char *name = HwAcc::codecName(id);
        return name ? QString::fromLatin1(name) : QString();
    }).join(','_q).toLatin1();
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
    return d->renderer->avgfps();
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
        d->renderer->resetTimings();
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
                u"vaapi"_q, u"vdpau"_q, u"vda"_q
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

SIA _IsAlphabet(ushort c) -> bool
{return _InRange<ushort>('a', c, 'z') || _InRange<ushort>('A', c, 'Z');}

SIA _IsAlphabet(const QString &text) -> bool
{
    for (auto &c : text) {
        if (!_IsAlphabet(c.unicode()))
            return false;
    }
    return true;
}

auto PlayEngine::exec() -> void
{
    _Debug("Start playloop thread");
    d->quit = false;
    int position = 0, cache = -2, duration = 0;
    bool first = false, loaded = false;
    Mrl mrl;
    QMap<QByteArray, QByteArray> leftmsg;

    auto metaData = [&] () {
        auto list = d->getmpv<QVariant>("metadata").toList();
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
                    chapter.m_time = time(map[u"time"_q].toDouble());
                    chapter.m_name = map[u"title"_q].toString();
                    if (chapter.m_name.isEmpty())
                        chapter.m_name = _MSecToString(chapter.m_time,
                                                       u"hh:mm:ss.zzz"_q);
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
            if (position > 0 && cache >= -1) {
                qint64 newCache = -2;
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
            position = -1;
            cache = -1;
            mrl = d->startInfo.mrl;
            d->postState(Loading);
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
            d->cache = -2;
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
                if (_InRange(2, stream.m_lang.size(), 3)
                        && _IsAlphabet(stream.m_lang))
                    stream.m_lang = translator_display_language(stream.m_lang);
                stream.m_title = map[u"title"_q].toString();
                stream.m_fileName = map[u"external-filename"_q].toString();
                if (!stream.m_fileName.isEmpty())
                    stream.m_title = QFileInfo(stream.m_fileName).fileName();
                stream.m_selected = map[u"selected"_q].toBool();
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
            d->postState(byCache ? Buffering : paused ? Paused : Playing);
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
            audio->m_device = d->audioDevice;
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
            video->m_input->m_type = vin[u"pixelformat"_q].toString();
            video->m_input->m_size.rwidth() = vin[u"w"_q].toInt();
            video->m_input->m_size.rheight() = vin[u"h"_q].toInt();
            video->m_input->m_fps = d->getmpv<double>("fps");
            video->m_output->m_type = vout[u"pixelformat"_q].toString();
            video->m_output->m_size.rwidth() = vout[u"dw"_q].toInt();
            video->m_output->m_size.rheight() = vout[u"dh"_q].toInt();
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
        d->postState(PlayEngine::Paused);
    else
        d->setmpv("pause", true);
}

auto PlayEngine::unpause() -> void
{
    if (d->hasImage)
        d->postState(PlayEngine::Playing);
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

auto PlayEngine::setVideoRenderer(VideoRenderer *renderer) -> void
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
    return d->renderer->droppedFrames();
}

auto PlayEngine::bitrate(double fps) const -> double
{
    return d->videoFormat.bitrate(fps);
}

auto PlayEngine::videoFormat() const -> VideoFormat
{
    return d->videoFormat;
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
        d->tellmpv("af", "set"_b, d->af());
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
    d->renderer->setDeintOptions(swdec, hwdec);
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

auto PlayEngine::subtitleFiles() const -> QVector<SubtitleFileInfo>
{
    return d->subtitleFiles;
}

auto PlayEngine::audioDeviceList() const -> QList<QPair<QString, QString>>
{
    const QVariantList list = d->getmpv<QVariant>("audio-device-list").toList();
    QList<QPair<QString, QString>> devs;
    devs.reserve(list.size());
    for (auto &one : list) {
        const auto map = one.toMap();
        devs.append({map[u"name"_q].toString(), map[u"description"_q].toString()});
    }
    return devs;
}
