#include "playengine.hpp"
#include <libmpv/opengl_cb.h>
#include "opengl/openglframebufferobject.hpp"
#include "audio/audionormalizeroption.hpp"
#include "playengine_p.hpp"

PlayEngine::PlayEngine()
: d(new Data(this)) {
    Q_ASSERT(d->confDir.isValid());

    _Debug("Create audio/video plugins");
    d->audio = new AudioController(this);
    d->video = new VideoRenderer;
    d->filter = new VideoFilter;

    d->video->setRenderFrameFunction([this] (OpenGLFramebufferObject *fbo)
        { d->renderVideoFrame(fbo); });
    //        connect(d->renderer, &VideoRenderer::formatChanged,
    //                [this] (const VideoFormat &format) {
    //            auto renderer = d->videoInfo.renderer();
    //            renderer->setImgFmt(format.params().imgfmt);
    //            renderer->setBppSize(format.size());
    //            renderer->setSize(format.displaySize());
    //        });

    d->chapterInfo = new ChapterInfoObject(this, this);
    d->updateMediaName();

    _Debug("Make registrations and connections");

//    connect(d->video, &VideoOutput::formatChanged,
//            this, &PlayEngine::updateVideoFormat);

    d->handle = mpv_create();
    d->client = mpv_client_name(d->handle);
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
    connect(this, &PlayEngine::beginChanged, this, &PlayEngine::endChanged);
    connect(this, &PlayEngine::durationChanged, this, &PlayEngine::endChanged);
    connect(this, &PlayEngine::videoStreamsChanged, this, [=] () {
        if (_Change(d->hasVideo, !d->streams[StreamVideo].tracks.isEmpty()))
            emit hasVideoChanged();
        d->videoInfo.setTracks(d->streams[StreamVideo].tracks);
    });
    connect(this, &PlayEngine::audioStreamsChanged, this, [=] ()
        { d->audioInfo.setTracks(d->streams[StreamAudio].tracks); });
    connect(this, &PlayEngine::subtitleStreamsChanged, this, [=] ()
        { d->subInfo.setTracks(d->streams[StreamSubtitle].tracks); });
    connect(this, &PlayEngine::currentVideoStreamChanged, this, [=] (int id)
        { d->videoInfo.setTrack(d->streams[StreamVideo].tracks.value(id)); });
    connect(this, &PlayEngine::currentAudioStreamChanged, this, [=] (int id)
        { d->audioInfo.setTrack(d->streams[StreamAudio].tracks.value(id)); });
    connect(this, &PlayEngine::currentSubtitleStreamChanged, this, [=] ()
        { d->subInfo.setTracks(d->streams[StreamSubtitle].tracks); });
    auto checkDeint = [=] () {
        auto act = Unavailable;
        if (d->filter->isInputInterlaced())
            act = d->filter->isOutputInterlaced() ? Deactivated : Activated;
        d->videoInfo.setDeinterlacer(act);
    };
    connect(d->filter, &VideoFilter::inputInterlacedChanged,
            this, checkDeint, Qt::QueuedConnection);
    connect(d->filter, &VideoFilter::outputInterlacedChanged,
            this, checkDeint, Qt::QueuedConnection);
    connect(d->audio, &AudioController::inputFormatChanged, this, [=] () {
        d->audioInfo.output()->setFormat(d->audio->inputFormat());
    }, Qt::QueuedConnection);
    connect(d->audio, &AudioController::outputFormatChanged, this, [=] () {
        d->audioInfo.renderer()->setFormat(d->audio->outputFormat());
    }, Qt::QueuedConnection);
    connect(d->audio, &AudioController::samplerateChanged, this, [=] (int sr) {
        d->audioInfo.renderer()->setSampleRate(sr, true);
    }, Qt::QueuedConnection);
    connect(d->audio, &AudioController::gainChanged,
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
    setOption("vo", d->vo());
    setOption("fixed-vo", "yes");
    auto hwdec = HwAcc::name();
    setOption("hwdec", hwdec.isEmpty() ? "no" : hwdec.toLatin1().constData());

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
    d->hook();

    auto ptr = mpv_get_sub_api(d->handle, MPV_SUB_API_OPENGL_CB);
    d->glMpv = static_cast<mpv_opengl_cb_context*>(ptr);
    Q_ASSERT(d->glMpv);
    auto cbUpdate = [] (void *priv) {
        auto p = static_cast<PlayEngine*>(priv);
        if (p->d->video)
            p->d->video->updateForNewFrame(p->d->videoInfo.renderer()->size());
    };
    mpv_opengl_cb_set_update_callback(d->glMpv, cbUpdate, this);

    d->fpsMeasure.setTimer([=]()
        { d->videoInfo.renderer()->setFps(d->fpsMeasure.get()); }, 100000);
}

PlayEngine::~PlayEngine()
{
    d->initialized = false;
    mpv_terminate_destroy(d->handle);
    delete d->chapterInfo;
    delete d->audio;
    delete d->video;
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
    auto err = mpv_opengl_cb_init_gl(d->glMpv, nullptr, getProcAddr, ctx);
    Q_ASSERT(err >= 0);
}

auto PlayEngine::finalizeGL(QOpenGLContext */*ctx*/) -> void
{
    mpv_opengl_cb_uninit_gl(d->glMpv);
}

auto PlayEngine::metaData() const -> const MetaData&
{
    return d->metaData;
}

auto PlayEngine::updateVideoFormat(VideoFormat format) -> void
{
    if (_Change(d->videoFormat, format))
        emit videoFormatChanged(d->videoFormat);
}

auto PlayEngine::setSubtitleDelay(int ms) -> void
{
    if (_Change(d->subDelay, ms))
        d->setmpv_async("sub-delay", d->subDelay/1000.0);
}

auto PlayEngine::setSubtitleFiles(const StreamList &files) -> void
{
    d->subInfo.setFiles(files);
}

auto PlayEngine::chapterInfo() const -> ChapterInfoObject*
{
    return d->chapterInfo;
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

auto PlayEngine::setSubtitlePriority(const QStringList &sp) -> void
{
    d->streams[StreamSubtitle].priority = sp;
}

auto PlayEngine::setAudioPriority(const QStringList &ap) -> void
{
    d->streams[StreamAudio].priority = ap;
}

auto PlayEngine::subtitleStreams() const -> const StreamList&
{
    return d->streams[StreamSubtitle].tracks;
}

auto PlayEngine::videoRenderer() const -> VideoRenderer*
{
    return d->video;
}

auto PlayEngine::videoStreams() const -> const StreamList&
{
    return d->streams[StreamVideo].tracks;
}

auto PlayEngine::audioSync() const -> int
{
    return d->audioSync;
}

auto PlayEngine::audioStreams() const -> const StreamList&
{
    return d->streams[StreamAudio].tracks;
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

auto PlayEngine::setSubtitleStyle(const OsdTheme &style) -> void
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
    if (_Change(d->layout, layout) && d->position > 0) {
        d->setmpv("options/audio-channels", ChannelLayoutInfo::data(d->layout));
        d->tellmpv("ao_reload");
    }
}

auto PlayEngine::setAudioDevice(const QString &device) -> void
{
    d->audioDevice = device;
}

auto PlayEngine::screen() const -> QQuickItem*
{
    return d->video;
}

auto PlayEngine::setMinimumCache(qreal playback, qreal seeking) -> void
{
    d->cacheForPlayback = playback;
    d->cacheForSeeking = seeking;
}

auto PlayEngine::volumeNormalizer() const -> double
{
    auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
}

auto PlayEngine::setHwAcc(bool use, const QStringList &codecs) -> void
{
    d->hwCodecs = codecs.join(','_q).toLatin1();
    d->useHwAcc = use;
}

auto PlayEngine::isSubtitleStreamsVisible() const -> bool
{
    return d->subStreamsVisible;
}

auto PlayEngine::setSubtitleStreamsVisible(bool visible) -> void
{
    d->subStreamsVisible = visible;
    const auto id = currentSubtitleStream();
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id >= 0));
}

auto PlayEngine::setCurrentSubtitleStream(int id, bool reserve) -> void
{
    d->setmpv_async("sub-visibility", (d->subStreamsVisible && id > 0));
    if (id > 0) {
        d->setmpv_async("sub", id);
        if (reserve)
            d->streams[StreamSubtitle].reserved = id;
    }
}

auto PlayEngine::currentSubtitleStream() const -> int
{
    return d->currentTrack(StreamSubtitle);
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
    auto &tracks = d->streams[StreamSubtitle].tracks;
    auto it = tracks.find(id);
    if (it != tracks.end()) {
        if (it->isExternal()) {
            for (int i=0; i<d->subtitleFiles.size(); ++i) {
                if (d->subtitleFiles[i].path == it->m_fileName)
                    d->subtitleFiles.removeAt(i);
            }
        }
        d->tellmpv("sub_remove", id);
    }
}

auto PlayEngine::avSync() const -> int
{
    return d->avSync;
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
    while (!d->quit)
        d->dispatch(mpv_wait_event(d->handle, 0.005));
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
    return d->hasVideo;
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
    return d->currentTrack(StreamAudio);
}

auto PlayEngine::setCurrentVideoStream(int id) -> void
{
    if (d->streams[StreamVideo].tracks.contains(id))
        d->setmpv_async("video", id);
}

auto PlayEngine::currentVideoStream() const -> int
{
    return d->currentTrack(StreamVideo);
}

auto PlayEngine::setCurrentAudioStream(int id, bool reserve) -> void
{
    if (d->streams[StreamAudio].tracks.contains(id)) {
        d->setmpv_async("audio", id);
        if (reserve)
            d->streams[StreamAudio].reserved = id;
    }
}

auto PlayEngine::setAudioSync(int sync) -> void
{
    if (_Change(d->audioSync, sync))
        d->setmpv_async("audio-delay", sync*0.001);
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

auto PlayEngine::setYouTube(YouTubeDL *yt) -> void
{
    d->youtube = yt;
}

auto PlayEngine::setColorRange(ColorRange range) -> void
{
    if (_Change(d->colorRange, range))
        d->setmpv_async("colormatrix-input-range", _EnumData(range).option);
}

auto PlayEngine::setColorSpace(ColorSpace space) -> void
{
    if (_Change(d->colorSpace, space))
        d->setmpv_async("colormatrix", _EnumData(space).option);
}

auto PlayEngine::setInterpolator(Interpolator type) -> void
{
    if (_Change(d->lscale, type))
        d->updateVideoSubOptions();
}

auto PlayEngine::setChromaUpscaler(Interpolator type) -> void
{
    if (_Change(d->cscale, type))
        d->updateVideoSubOptions();
}

auto PlayEngine::interpolator() const -> Interpolator
{
    return d->lscale;
}

auto PlayEngine::chromaUpscaler() const -> Interpolator
{
    return d->cscale;
}

auto PlayEngine::dithering() const -> Dithering
{
    return d->dithering;
}

auto PlayEngine::setDithering(Dithering dithering) -> void
{
    if (_Change(d->dithering, dithering))
        d->updateVideoSubOptions();
}

auto PlayEngine::videoEqualizer() const -> VideoColor
{
    return d->videoEq;
}

auto PlayEngine::setVideoEqualizer(const VideoColor &eq) -> void
{
    if (_Change(d->videoEq, eq)) {
        d->updateColorMatrix();
        d->updateVideoSubOptions();
    }
}

auto PlayEngine::setVideoEffects(VideoEffects e) -> void
{
    if (_Change(d->videoEffects, e)) {
        d->video->setFlipped(e & VideoEffect::FlipH, e & VideoEffect::FlipV);
        d->updateColorMatrix();
        d->updateVideoSubOptions();
    }
}

auto PlayEngine::videoEffects() const -> VideoEffects
{
    return d->videoEffects;
}
