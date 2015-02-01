#include "playengine.hpp"
#include "playengine_p.hpp"
#include "audio/audionormalizeroption.hpp"
#include "subtitle/subtitlemodel.hpp"

PlayEngine::PlayEngine()
: d(new Data(this)) {
    Q_ASSERT(d->confDir.isValid());

    _Debug("Create audio/video plugins");
    d->ac = new AudioController(this);
    d->vr = new VideoRenderer;
    d->filter = new VideoFilter;
    d->sr = new SubtitleRenderer;
    d->vr->setOverlay(d->sr);
    d->vr->setRenderFrameFunction([this] (OpenGLFramebufferObject *fbo)
        { d->renderVideoFrame(fbo); });

    connect(this, &PlayEngine::tick, this, [=] () {
        if (_Change(d->time_s, d->position/1000))
            emit time_sChanged();
        d->sr->render(d->position);
    });
    connect(this, &PlayEngine::durationChanged, this, [=] () {
        if (_Change(d->duration_s, d->duration/1000))
            emit duration_sChanged();
    });
    connect(this, &PlayEngine::beginChanged, this, [=] () {
        if (_Change(d->begin_s, d->begin/1000))
            emit begin_sChanged();
    });
    connect(this, &PlayEngine::endChanged, this, [=] () {
        if (_Change(d->end_s, end()/1000))
            emit end_sChanged();
    });

    connect(&d->params, &MrlState::sub_sync_changed,
            this, [=] (int ms) { d->sr->setDelay(ms); });

    connect(d->sr, &SubtitleRenderer::modelsChanged,
            this, &PlayEngine::subtitleModelsChanged);

    d->updateMediaName();

    _Debug("Make registrations and connections");

    d->handle = mpv_create();
    QByteArray loglv = "no";
    switch (Log::maximumLevel()) {
    case Log::Trace: loglv = "trace"; break;
    case Log::Debug: loglv = "v";     break;
    case Log::Info:  loglv = "info";  break;
    case Log::Warn:  loglv = "warn";  break;
    case Log::Error: loglv = "error"; break;
    case Log::Fatal: loglv = "fatal"; break;
    }
    mpv_request_log_messages(d->handle, loglv.constData());

    d->observe();
    connect(d->filter, &VideoFilter::skippingChanged, this, [=] (bool skipping) {
        if (skipping) {
            d->pauseAfterSkip = isPaused();
            d->setmpv_async("mute", true);
            d->setmpv_async("pause", false);
            d->setmpv_async("speed", 100.0);
        } else {
            d->setmpv_async("speed", d->params.play_speed() / 100.0);
            d->setmpv_async("pause", d->pauseAfterSkip);
            d->setmpv_async("mute", d->params.audio_muted());
        }
        d->updateVideoSubOptions();
        d->post(Searching, skipping);
    }, Qt::DirectConnection);
    connect(d->filter, &VideoFilter::seekRequested, this,
            &PlayEngine::seek);
    connect(this, &PlayEngine::beginChanged, this, &PlayEngine::endChanged);
    connect(this, &PlayEngine::durationChanged, this, &PlayEngine::endChanged);


    connect(&d->params, &MrlState::video_tracks_changed, this, [=] (StreamList list) {
        if (_Change(d->hasVideo, !list.isEmpty()))
            emit hasVideoChanged();
        d->videoInfo.setTracks(list);
    });
    connect(&d->params, &MrlState::audio_tracks_changed, this,
            [=] (StreamList list) { d->audioInfo.setTracks(list); });
    auto set_subs = [=] () {
        d->subInfo.setTracks(d->params.sub_tracks(),
                             d->params.sub_tracks_inclusive());
    };
    connect(&d->params, &MrlState::sub_tracks_changed, this, set_subs);
    connect(&d->params, &MrlState::sub_tracks_inclusive_changed, this, set_subs);

    auto checkDeint = [=] () {
        auto act = Unavailable;
        if (d->filter->isInputInterlaced())
            act = d->filter->isOutputInterlaced() ? Deactivated : Activated;
        d->videoInfo.setDeinterlacer(act);
    };
    connect(d->filter, &VideoFilter::inputInterlacedChanged,
            this, checkDeint);
    connect(d->filter, &VideoFilter::outputInterlacedChanged,
            this, checkDeint);
    connect(d->ac, &AudioController::inputFormatChanged, this, [=] () {
        d->audioInfo.output()->setFormat(d->ac->inputFormat());
    });
    connect(d->ac, &AudioController::outputFormatChanged, this, [=] () {
        d->audioInfo.renderer()->setFormat(d->ac->outputFormat());
    });
    connect(d->ac, &AudioController::samplerateChanged, this, [=] (int sr) {
        d->audioInfo.renderer()->setSampleRate(sr, true);
    });
    connect(d->ac, &AudioController::gainChanged,
            &d->audioInfo, &AudioInfoObject::setNormalizer);
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
    setOption("vo", d->vo(&d->params));
    setOption("af", d->af(&d->params));
    setOption("vf", d->vf(&d->params));
    setOption("fixed-vo", "yes");
    auto hwdec = HwAcc::name();
    setOption("hwdec", hwdec.isEmpty() ? "no" : hwdec.toLatin1().constData());

    auto overrides = qgetenv("BOMI_MPV_OPTIONS").trimmed();
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
    d->hook();

    auto ptr = mpv_get_sub_api(d->handle, MPV_SUB_API_OPENGL_CB);
    d->gl = static_cast<mpv_opengl_cb_context*>(ptr);
    Q_ASSERT(d->gl);
    auto cbUpdate = [] (void *priv) {
        auto p = static_cast<PlayEngine*>(priv);
        if (p->d->vr)
            p->d->vr->updateForNewFrame(p->d->videoInfo.renderer()->size());
    };
    mpv_opengl_cb_set_update_callback(d->gl, cbUpdate, this);

    d->fpsMeasure.setTimer([=]()
        { d->videoInfo.renderer()->setFps(d->fpsMeasure.get()); }, 100000);

    d->params.m_mutex = &d->mutex;
}

PlayEngine::~PlayEngine()
{
    d->params.m_mutex = nullptr;
    d->initialized = false;
    mpv_terminate_destroy(d->handle);
    qDeleteAll(d->chapterInfoList);
    delete d->ac;
    d->vr->setOverlay(nullptr);
    delete d->sr;
    delete d->vr;
    delete d->filter;
    delete d;
    _Debug("Finalized");
}

auto PlayEngine::initializeGL(QOpenGLContext *ctx) -> void
{
    auto getProcAddr = [] (void *ctx, const char *name) -> void* {
        auto gl = static_cast<QOpenGLContext*>(ctx);
        if (!gl)
            return nullptr;
        return reinterpret_cast<void*>(gl->getProcAddress(QByteArray(name)));
    };
    auto err = mpv_opengl_cb_init_gl(d->gl, nullptr, getProcAddr, ctx);
    Q_ASSERT(err >= 0);
}

auto PlayEngine::finalizeGL(QOpenGLContext */*ctx*/) -> void
{
    mpv_opengl_cb_uninit_gl(d->gl);
}

auto PlayEngine::metaData() const -> const MetaData&
{
    return d->metaData;
}

auto PlayEngine::setSubtitleDelay(int ms) -> void
{
    if (d->params.set_sub_sync(ms))
        d->setmpv_async("sub-delay", ms * 1e-3);
}

auto PlayEngine::mediaName() const -> QString
{
    return d->mediaName;
}

auto PlayEngine::cacheSize() const -> int
{
    return d->cacheSize;
}

auto PlayEngine::cacheUsed() const -> int
{
    return d->cacheUsed;
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

auto PlayEngine::setPriority_locked(const QStringList &audio, const QStringList &sub) -> void
{
    d->streams[StreamAudio].priority = audio;
    d->streams[StreamSubtitle].priority = sub;
}

auto PlayEngine::setAutoloader_locked(const Autoloader &audio, const Autoloader &sub) -> void
{
    d->streams[StreamAudio].autoloader = audio;
    d->streams[StreamSubtitle].autoloader = sub;
}

auto PlayEngine::setSubtitleInclusiveTrackSelected(int id, bool s) -> void
{
    if (s)
        d->sr->select(id);
    else
        d->sr->deselect(id);
    d->syncInclusiveSubtitles();
}

auto PlayEngine::setSubtitleTrackSelected(int id, bool s) -> void
{
    if (s)
        d->setmpv_async("sid", id);
    else if (d->params.sub_tracks().selectionId() == id)
        d->setmpv_async("sid", -1);
}

auto PlayEngine::autoloadSubtitleFiles() -> void
{
    clearSubtitleFiles();
    QStringList files; QVector<SubComp> loads;
    d->mutex.lock();
    _R(files, loads) = d->autoloadSubtitle(&d->params);
    d->mutex.unlock();
    for (auto &file : files) {
        d->setmpv_async("options/subcp", d->assEncodings[file].toLatin1());
        d->tellmpv_async("sub_add", file.toLocal8Bit(), "auto"_b);
    }
    d->setInclusiveSubtitles(loads);
}

auto PlayEngine::autoloadAudioFiles() -> void
{
    setAudioFiles(d->autoloadFiles(StreamAudio));
}

auto PlayEngine::reloadSubtitleFiles() -> void
{
    d->mutex.lock();
    auto old1 = d->params.sub_tracks();
    auto old2 = d->params.sub_tracks_inclusive();
    d->mutex.unlock();
    clearSubtitleFiles();
    for (auto &track : old1) {
        if (track.isExternal())
            d->sub_add(track.file(), track.encoding(), track.isSelected());
    }
    d->setInclusiveSubtitles(d->restoreInclusiveSubtitles(old2));
}

auto PlayEngine::reloadAudioFiles() -> void
{
    d->mutex.lock();
    auto old = d->params.audio_tracks();
    d->mutex.unlock();
    clearAudioFiles();
    for (auto &track : old) {
        if (track.isExternal())
            d->audio_add(track.file(), track.isSelected());
    }
}

auto PlayEngine::setSubtitleHidden(bool hidden) -> void
{
    if (d->params.set_sub_hidden(hidden))
        d->setmpv_async("sub-visibility", !hidden);
}

auto PlayEngine::setVideoTrackSelected(int id, bool s) -> void
{
    if (s)
        d->setmpv_async("vid", id);
    else if (d->params.video_tracks().selectionId() == id)
        d->setmpv_async("vid", -1);
}

auto PlayEngine::setAudioTrackSelected(int id, bool s) -> void
{
    if (s)
        d->setmpv_async("aid", id);
    else if (d->params.audio_tracks().selectionId() == id)
        d->setmpv_async("aid", -1);
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
    return d->params.play_speed() * 1e-2;
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

auto PlayEngine::setSpeedPercent(int p) -> void
{
    if (d->params.set_play_speed(qBound(1, p, 1000)))
        d->setmpv_async("speed", speed());
}

auto PlayEngine::setSubtitleStyle_locked(const OsdStyle &style) -> void
{
    d->sr->setStyle(style);

    const auto font = style.font;
    d->setmpv_async("options/sub-text-color", font.color.name(QColor::HexArgb).toLatin1());
    QStringList fontStyles;
    if (font.bold())
        fontStyles.append(u"Bold"_q);
    if (font.italic())
        fontStyles.append(u"Italic"_q);
    QString family = font.family();
    if (!fontStyles.isEmpty())
        family += ":style="_a % fontStyles.join(' '_q);
    const double factor = font.size * 720.0;
    d->setmpv_async("options/sub-text-font", family.toLatin1());
    d->setmpv_async("options/sub-text-font-size", factor);
    const auto &outline = style.outline;
    const auto scaled = [factor] (double v)
        { return qBound(0., v*factor, 10.); };
    const auto color = [] (const QColor &color)
        { return color.name(QColor::HexArgb).toLatin1(); };
    if (outline.enabled) {
        d->setmpv_async("options/sub-text-border-size", scaled(outline.width));
        d->setmpv_async("options/sub-text-border-color", color(outline.color));
    } else
        d->setmpv_async("options/sub-text-border-size", 0.0);
    const auto &bbox = style.bbox;
    if (bbox.enabled)
        d->setmpv_async("options/sub-text-back-color", color(bbox.color));
    else
        d->setmpv_async("options/sub-text-back-color", color(Qt::transparent));
    auto norm = [] (const QPointF &p) { return sqrt(p.x()*p.x() + p.y()*p.y()); };
    const auto &shadow = style.shadow;
    if (shadow.enabled) {
        d->setmpv_async("options/sub-text-shadow-color", color(shadow.color));
        d->setmpv_async("options/sub-text-shadow-offset", scaled(norm(shadow.offset)));
    } else {
        d->setmpv_async("options/sub-text-shadow-color", color(Qt::transparent));
        d->setmpv_async("options/sub-text-shadow-offset", 0.0);
    }
}

auto PlayEngine::seek(int pos) -> void
{
    d->chapter = -1;
    if (!d->hasImage)
        d->tellmpv("seek", (double)pos/1000.0, 2);
    d->filter->stopSkipping();
}

auto PlayEngine::relativeSeek(int pos) -> void
{
    if (!d->hasImage) {
        d->tellmpv("seek", (double)pos/1000.0, 0);
        emit sought();
    }
    d->filter->stopSkipping();
}

auto PlayEngine::setClippingMethod_locked(ClippingMethod method) -> void
{
    d->ac->setClippingMethod(method);
}

auto PlayEngine::setChannelLayoutMap_locked(const ChannelLayoutMap &map) -> void
{
    d->ac->setChannelLayoutMap(map);
}

auto PlayEngine::reload() -> void
{
    if (d->state == PlayEngine::Stopped)
        return;
//    info.reloaded = true;
//    load(info);
}

auto PlayEngine::setHistory(HistoryModel *history) -> void
{
    d->history = history;
}

auto PlayEngine::lock() -> void
{
    d->mutex.lock();
}

auto PlayEngine::unlock() -> void
{
    d->mutex.unlock();
}

auto PlayEngine::setChannelLayout(ChannelLayout layout) -> void
{
    if (d->params.set_audio_channel_layout(layout)) {
        d->setmpv_async("options/audio-channels", ChannelLayoutInfo::data(layout));
        if (d->position > 0)
            d->tellmpv_async("ao_reload");
    }
}

auto PlayEngine::setAudioDevice_locked(const QString &device) -> void
{
    d->params.d->audioDevice = device;
    d->setmpv_async("options/audio-device", d->params.d->audioDevice.toLatin1());
}

auto PlayEngine::screen() const -> QQuickItem*
{
    return d->vr;
}

auto PlayEngine::setCache_locked(const CacheInfo &info) -> void
{
    d->params.d->cache = info;
}

auto PlayEngine::setHwAcc_locked(bool use, const QStringList &codecs) -> void
{
    d->hwcdc = codecs.join(','_q).toLatin1();
    d->hwdec = use;
    d->setmpv_async("options/hwdec-codecs", use ? d->hwcdc : ""_b);
}

auto PlayEngine::avSync() const -> int
{
    return d->avSync;
}

auto PlayEngine::stepFrame(int direction) -> void
{
    if ((d->state & (Playing | Paused)) && d->seekable)
        d->tellmpv_async(direction > 0 ? "frame_step" : "frame_back_step");
}

auto PlayEngine::isWaiting() const -> bool
{
    return d->waitings;
}

auto PlayEngine::waiting() const -> Waiting
{
    if (d->waitings & Searching)
        return Searching;
    if (d->waitings & Buffering)
        return Buffering;
    if (d->waitings & Loading)
        return Loading;
    return NoWaiting;
}

auto PlayEngine::state() const -> State
{
    return d->state;
}

auto PlayEngine::customEvent(QEvent *event) -> void
{
    d->process(event);
}

auto PlayEngine::mediaInfo() const -> MediaInfoObject*
{
    return &d->mediaInfo;
}

auto PlayEngine::audioInfo() const -> AudioInfoObject*
{
    return &d->audioInfo;
}

auto PlayEngine::videoInfo() const -> VideoInfoObject*
{
    return &d->videoInfo;
}

auto PlayEngine::subInfo() const -> SubtitleInfoObject*
{
    return &d->subInfo;
}

auto PlayEngine::setCurrentChapter(int id) -> void
{
    d->setmpv_async("chapter", id);
}

auto PlayEngine::setCurrentEdition(int id, int from) -> void
{
    const auto mrl = d->mrl;
    if (id == DVDMenu && mrl.isDisc()) {
        static const char *cmds[] = {"discnav", "menu", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds),
                 "Couldn't send 'discnav menu'.");
    } else if (0 <= id && id < d->editions.size()) {
        d->setmpv_async(mrl.isDisc() ? "disc-title" : "edition", id);
        seek(from);
    }
}

auto PlayEngine::setAudioVolume(int volume) -> void
{
    if (d->params.set_audio_volume(qBound(0, volume, 100)))
        d->setmpv_async("volume", d->mpVolume());
}

auto PlayEngine::isMuted() const -> bool
{
    return d->params.audio_muted();
}

auto PlayEngine::volume() const -> int
{
    return d->params.audio_volume();
}

auto PlayEngine::setAudioAmpPercent(int amp) -> void
{
    if (d->params.set_audio_amplifier(qBound(1, amp, 1000)))
        d->setmpv_async("volume", d->mpVolume());
}

auto PlayEngine::setAudioMuted(bool muted) -> void
{
    if (d->params.set_audio_muted(muted))
        d->setmpv_async("mute", muted);
}

auto PlayEngine::exec() -> void
{
    _Debug("Start playloop thread");
    d->quit = false;
    while (!d->quit)
        d->dispatch(mpv_wait_event(d->handle, 0.005));
    _Debug("Finish playloop thread");
}

auto PlayEngine::shutdown() -> void
{
    d->tellmpv("quit 1");
}

auto PlayEngine::setResume_locked(bool resume) -> void
{
    d->resume = resume;
}

auto PlayEngine::setMrl(const Mrl &mrl) -> void
{
    if (d->mrl != mrl) {
        stop();
        d->mrl = mrl;
    }
}

auto PlayEngine::load(const Mrl &mrl) -> void
{
    if (_Change(d->mrl, mrl)) {
        d->hasImage = mrl.isImage();
        d->updateMediaName();
        emit mrlChanged(d->mrl);
    }
    if (!d->mrl.isEmpty())
        d->loadfile(d->mrl);
}

auto PlayEngine::time() const -> int
{
    return d->position;
}

auto PlayEngine::isSeekable() const -> bool
{
    return d->seekable;
}

auto PlayEngine::hasVideoFrame() const -> bool
{
    return d->vr->hasFrame();
}

auto PlayEngine::hasVideo() const -> bool
{
    return d->hasVideo;
}

auto PlayEngine::currentChapter() const -> int
{
    return d->chapter;
}

auto PlayEngine::pause() -> void
{
    if (d->hasImage)
        d->post(Paused);
    else
        d->setmpv("pause", true);
    d->pauseAfterSkip = true;
    d->filter->stopSkipping();
}

auto PlayEngine::unpause() -> void
{
    if (d->hasImage)
        d->post(Playing);
    else
        d->setmpv("pause", false);
}

auto PlayEngine::mrl() const -> Mrl
{
    return d->mrl;
}

auto PlayEngine::setAudioSync(int sync) -> void
{
    if (d->params.set_audio_sync(sync))
        d->setmpv_async("audio-delay", sync * 1e-3);
}

auto PlayEngine::setAudioVolumeNormalizer(bool on) -> void
{
    if (d->params.set_audio_volume_normalizer(on))
        d->ac->setNormalizerActivated(on);
}

auto PlayEngine::setAudioTempoScaler(bool on) -> void
{
    if (d->params.set_audio_tempo_scaler(on))
        d->tellmpv_async("af", "set"_b, d->af(&d->params));
}

auto PlayEngine::stop() -> void
{
    d->tellmpv("stop");
}

auto PlayEngine::setVolumeNormalizerOption_locked(const AudioNormalizerOption &option)
-> void
{
    d->ac->setNormalizerOption(option);
}

auto PlayEngine::setDeintOptions_locked(const DeintOption &swdec,
                                 const DeintOption &hwdec) -> void
{
    d->params.d->deint_swdec = swdec;
    d->params.d->deint_hwdec = hwdec;
    emit deintOptionsChanged();
}

auto PlayEngine::deintOptionForSwDec() const -> DeintOption
{
    return d->params.d->deint_swdec;
}

auto PlayEngine::deintOptionForHwDec() const -> DeintOption
{
    return d->params.d->deint_hwdec;
}

auto PlayEngine::videoSizeHint() const -> QSize
{
    return d->vr->sizeHint();
}

auto PlayEngine::setVideoOffset(const QPoint &offset) -> void
{
    if (d->params.set_video_offset(offset))
        d->vr->setOffset(offset);
}

auto PlayEngine::setVideoAspectRatio(VideoRatio ratio) -> void
{
    if (d->params.set_video_aspect_ratio(ratio))
        d->vr->setAspectRatio(_EnumData(ratio));
}

auto PlayEngine::setVideoCropRatio(VideoRatio ratio) -> void
{
    if (d->params.set_video_crop_ratio(ratio))
        d->vr->setCropRatio(_EnumData(ratio));
}

auto PlayEngine::setVideoVerticalAlignment(VerticalAlignment a) -> void
{
    if (d->params.set_video_vertical_alignment(a)) {
        const auto v = _EnumData(d->params.video_vertical_alignment());
        const auto h = _EnumData(d->params.video_horizontal_alignment());
        d->vr->setAlignment(v | h);
    }
}

auto PlayEngine::setVideoHorizontalAlignment(HorizontalAlignment a) -> void
{
    if (d->params.set_video_horizontal_alignment(a)) {
        const auto v = _EnumData(d->params.video_vertical_alignment());
        const auto h = _EnumData(d->params.video_horizontal_alignment());
        d->vr->setAlignment(v | h);
    }
}

auto PlayEngine::setDeintMode(DeintMode mode) -> void
{
    if (d->params.set_video_deinterlacing(mode)) {
        if (isPaused()) {
            d->setmpv_async("deinterlace", !!(int)mode);
            d->refresh();
        } else
            d->setmpv_async("deinterlace", !!(int)mode);
    }
}

auto PlayEngine::deintMode() const -> DeintMode
{
    return d->params.video_deinterlacing();
}

auto PlayEngine::sendMouseClick(const QPointF &pos) -> void
{
    if (d->setMousePos(pos)) {
        static const char *cmds[] = {"discnav", "mouse", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds), "Couldn't send mouse.");
    }
}

auto PlayEngine::sendMouseMove(const QPointF &pos) -> void
{
    if (d->setMousePos(pos)) {
        static const char *cmds[] = {"discnav", "mouse_move", nullptr};
        d->check(mpv_command_async(d->handle, 0, cmds),
                 "Couldn't send mouse_move.");
    }
}

auto PlayEngine::audioDeviceList() const -> QList<AudioDevice>
{
    const QVariantList list = d->getmpv<QVariant>("audio-device-list").toList();
    QList<AudioDevice> devs;
    devs.reserve(list.size());
    for (auto &one : list) {
        const auto map = one.toMap();
        AudioDevice dev;
        dev.name = map[u"name"_q].toString();
        dev.description = map[u"description"_q].toString();
        devs.push_back(dev);
    }
    return devs;
}

auto PlayEngine::setYle(YleDL *yle) -> void
{
    d->yle = yle;
}

auto PlayEngine::setYouTube(YouTubeDL *yt) -> void
{
    d->youtube = yt;
}

auto PlayEngine::setColorRange(ColorRange range) -> void
{
    if (d->params.set_video_range(range))
        d->setmpv_async("colormatrix-input-range", _EnumData(range).option);
}

auto PlayEngine::setColorSpace(ColorSpace space) -> void
{
    if (d->params.set_video_space(space))
        d->setmpv_async("colormatrix", _EnumData(space).option);
}

auto PlayEngine::setInterpolator(Interpolator type) -> void
{
    if (d->params.set_video_interpolator(type))
        d->updateVideoSubOptions();
}

auto PlayEngine::setChromaUpscaler(Interpolator type) -> void
{
    if (d->params.set_video_chroma_upscaler(type))
        d->updateVideoSubOptions();
}

auto PlayEngine::setVideoDithering(Dithering dithering) -> void
{
    if (d->params.set_video_dithering(dithering))
        d->updateVideoSubOptions();
}

auto PlayEngine::setVideoEqualizer(const VideoColor &eq) -> void
{
    if (d->params.set_video_color(eq)) {
        d->updateColorMatrix();
        d->updateVideoSubOptions();
    }
}

auto PlayEngine::setVideoEffects(VideoEffects e) -> void
{
    if (d->params.set_video_effects(e)) {
        d->vr->setFlipped(e & VideoEffect::FlipH, e & VideoEffect::FlipV);
        d->updateColorMatrix();
        d->updateVideoSubOptions();
    }
}

auto PlayEngine::takeSnapshot(Snapshot mode) -> void
{
    d->snapshot = mode;
    d->vr->updateForNewFrame(d->displaySize());
}

auto PlayEngine::snapshot(bool withOsd) -> QImage
{
    return withOsd ? d->ssWithOsd : d->ssNoOsd;
}

auto PlayEngine::clearSnapshots() -> void
{
    d->ssNoOsd = d->ssWithOsd = QImage();
}

auto PlayEngine::setVideoHighQualityDownscaling(bool on) -> void
{
    if (d->params.set_video_hq_downscaling(on))
        d->updateVideoSubOptions();
}

auto PlayEngine::setVideoHighQualityUpscaling(bool on) -> void
{
    if (d->params.set_video_hq_upscaling(on))
        d->updateVideoSubOptions();
}

auto PlayEngine::seekToNextBlackFrame() -> void
{
    if (!isStopped())
        d->filter->skipToNextBlackFrame();
}

auto PlayEngine::waitingText() const -> QString
{
    switch (waiting()) {
    case Loading:
        return tr("Loading");
    case Searching:
        return tr("Searching");
    case Buffering:
        return tr("Buffering");
    case Seeking:
        return tr("Seeking");
    default:
        return QString();
    }
}

auto PlayEngine::stateText() const -> QString
{
    switch (d->state) {
    case Stopped:
        return tr("Stopped");
    case Playing:
        return tr("Playing");
    case Paused:
        return tr("Paused");
    case Error:
        return tr("Error");
    default:
        return QString();
    }
}

auto PlayEngine::setAudioEqualizer(const AudioEqualizer &eq) -> void
{
    d->ac->setEqualizer(eq);
}

auto PlayEngine::chapterInfoList() const -> QQmlListProperty<ChapterInfoObject>
{
    return _MakeQmlList(this, &d->chapterInfoList);
}

auto PlayEngine::time_s() const -> int
{
    return d->time_s;
}

auto PlayEngine::duration_s() const -> int
{
    return d->duration_s;
}

auto PlayEngine::begin_s() const -> int
{
    return d->begin_s;
}

auto PlayEngine::end_s() const -> int
{
    return d->end_s;
}

auto PlayEngine::setAudioFiles(const QStringList &files) -> void
{
    clearAudioFiles();
    addAudioFiles(files);
}

auto PlayEngine::addAudioFiles(const QStringList &files) -> void
{
    for (auto file : files)
        d->tellmpv_async("audio_add", file, "auto"_b);
}

auto PlayEngine::clearAudioFiles() -> void
{
    for (auto &track : d->params.audio_tracks()) {
        if (track.isExternal())
            d->tellmpv_async("audio_remove", track.id());
    }
}

auto PlayEngine::params() const -> const MrlState*
{
    return &d->params;
}

auto PlayEngine::setSubtitleEncoding_locked(const QString &enc, double accuracy) -> void
{
    d->params.d->subtitleEncoding = enc;
    d->params.d->autodetect = accuracy;
}

auto PlayEngine::setAutoselectMode_locked(bool enable, AutoselectMode mode,
                                       const QString &ext) -> void
{
    d->params.d->autoselect = enable;
    d->params.d->autoselectMode = mode;
    d->params.d->autoselectExt = ext;
}

auto PlayEngine::setSubtitleAlignment(VerticalAlignment a) -> void
{
    if (d->params.set_sub_alignment(a))
        d->sr->setTopAligned(a == VerticalAlignment::Top);
}

auto PlayEngine::setSubtitlePosition(int pos) -> void
{
    if (d->params.set_sub_position(pos))
        d->sr->setPos(pos * 0.01);
}

auto PlayEngine::captionBeginTime() -> int
{
    return d->sr->start(d->position);
}

auto PlayEngine::captionEndTime() -> int
{
    return d->sr->finish(d->position);
}

auto PlayEngine::seekCaption(int direction) -> void
{
    int time = -1;
    if (direction < 0)
        time = d->sr->previous();
    else if (direction > 0)
        time = d->sr->next();
    else
        time = d->sr->current();
    if (time > 0)
        seek(time);
}

auto PlayEngine::subtitleImage(const QRect &rect, QRectF *subRect) const -> QImage
{
    return d->sr->draw(rect, subRect);
}

auto PlayEngine::setSubtitleDisplay(SubtitleDisplay sd) -> void
{
    if (d->params.set_sub_display(sd))
        d->vr->setOverlayOnLetterbox(sd == SubtitleDisplay::OnLetterbox);
}

auto PlayEngine::setSubtitleFiles(const QStringList &files,
                                  const QString &encoding) -> void
{
    clearSubtitleFiles();
    addSubtitleFiles(files, encoding);
}

auto PlayEngine::setSubtitleFiles(const QVector<StringPair> &fileEnc) -> void
{
    clearSubtitleFiles();
    addSubtitleFiles(fileEnc);
}

auto PlayEngine::addSubtitleFiles(const QStringList &files,
                                  const QString &encoding) -> void
{
    QVector<StringPair> fileEnc(files.size());
    for (int i = 0; i < files.size(); ++i) {
        fileEnc[i].s1 = files[i];
        fileEnc[i].s2 = encoding;
    }
    addSubtitleFiles(fileEnc);
}

auto PlayEngine::addSubtitleFiles(const QVector<StringPair> &fileEncs) -> void
{
    if (fileEncs.isEmpty())
        return;
    QVector<SubComp> loaded;
    for (auto &fe : fileEncs) {
        const auto &file = fe.s1;
        const auto enc = d->params.d->detect(file, fe.s2);
        Subtitle sub;
        if (sub.load(file, enc, -1)) {
            for (int i = 0; i < sub.size(); ++i) {
                loaded.push_back(sub[i]);
                loaded.back().selection() = true;
            }
        } else {
            d->setmpv_async("options/subcp", enc.toLatin1());
            d->tellmpv_async("sub_add", file.toLocal8Bit(), "auto"_b);
            d->mutex.lock();
            d->assEncodings[file] = enc;
            d->mutex.unlock();
        }
    }
    d->sr->addComponents(loaded);
    d->syncInclusiveSubtitles();
}

auto PlayEngine::clearSubtitleFiles() -> void
{
    for (auto &track : d->params.sub_tracks()) {
        if (track.isExternal())
            d->tellmpv_async("sub_remove", track.id());
    }
    d->setInclusiveSubtitles(QVector<SubComp>());
}
