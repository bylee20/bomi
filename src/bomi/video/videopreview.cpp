#include "videopreview.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/openglframebufferobject.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "misc/dataevent.hpp"
#include "misc/log.hpp"
#include "player/mpv.hpp"
#include <QQuickWindow>

DECLARE_LOG_CONTEXT(Video)

enum EventType {NewFrame = QEvent::User + 1 };

struct VideoPreview::Data {
    VideoPreview *p = nullptr;
    int id = 0;
    bool redraw = false, active = false, keyframe = true, video = false;
    bool loaded = false;
    QSize displaySize{0, 0};
    double rate = 0.0, aspect = 0, percent = 0;
    Mpv mpv;
    auto vo() const -> QByteArray { return "opengl-cb"_b; }
    auto hasVideo() -> bool { return id > 0 && !displaySize.isEmpty(); }
    auto sizeAspect() const -> double
    {
        if (displaySize.isEmpty())
            return 0;
        return displaySize.width()/(double)displaySize.height();
    }
};

VideoPreview::VideoPreview(QQuickItem *parent)
    : Super(parent), d(new Data)
{
    d->p = this;
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsDrops, true);

    d->mpv.setLogContext("mpv/preview"_b);

    d->mpv.create();

    d->mpv.setObserver(this);
    d->mpv.observe("vid", [=] (int id) {
        if (_Change(d->id, id) && _Change(d->video, d->hasVideo()))
            emit hasVideoChanged(d->video);
    });
    d->mpv.request(MPV_EVENT_START_FILE, [=] () { d->loaded = true; });
    d->mpv.request(MPV_EVENT_END_FILE, [=] () { d->loaded = false; });
    d->mpv.setOption("hwdec", "no");
    d->mpv.setOption("aid", "no");
    d->mpv.setOption("sid", "no");
    d->mpv.setOption("audio-file-auto", "no");
    d->mpv.setOption("sub-auto", "no");
    d->mpv.setOption("osd-level", "0");
    d->mpv.setOption("quiet", "yes");
    d->mpv.setOption("title", "\"\"");
    d->mpv.setOption("audio-pitch-correction", "no");
    d->mpv.setOption("vo", d->vo());
    d->mpv.setOption("pause", "yes");
    d->mpv.setOption("keep-open", "always");
    d->mpv.setOption("vd-lavc-skiploopfilter", "all");
    d->mpv.setOption("use-text-osd", "no");
    d->mpv.setOption("audio-display", "no");
    d->mpv.initialize(Log::Error);
    d->mpv.setUpdateCallback([=] () { _PostEvent(this, NewFrame); });

    d->mpv.start();
}

VideoPreview::~VideoPreview() {
    d->mpv.destroy();
    delete d;
}

auto VideoPreview::initializeGL() -> void
{
    Super::initializeGL();
    d->mpv.initializeGL(QOpenGLContext::currentContext());
}

auto VideoPreview::finalizeGL() -> void
{
    Super::finalizeGL();
    d->mpv.finalizeGL();
}

auto VideoPreview::rate() const -> double
{
    return d->rate;
}

auto VideoPreview::setRate(double rate) -> void
{
    if (!d->active || !d->video || !d->loaded)
        return;
    if (_Change(d->rate, rate)) {
        if (_Change(d->percent, qRound(d->rate * 10000)/100.0))
            d->mpv.tellAsync("seek", d->percent, d->keyframe
                             ? "absolute-percent+keyframes"_b
                             : "absolute-percent+exact"_b);
        emit rateChanged(d->rate);
    }
}

auto VideoPreview::aspectRatio() const -> double
{
    return d->aspect;
}

auto VideoPreview::customEvent(QEvent *event) -> void
{
    switch (static_cast<int>(event->type())) {
    case NewFrame: {
        d->redraw = true;
        reserve(UpdateMaterial);
        break;
    } default:
        d->mpv.process(event);
        break;
    }
}

auto VideoPreview::hasVideo() const -> bool
{
    return d->video;
}

auto VideoPreview::setSizeHint(const QSize &size) -> void
{
    const auto dp = _Change(d->displaySize, size);
    const auto as = _Change(d->aspect, d->sizeAspect());
    const auto hv = _Change(d->video, d->hasVideo());
    if (dp)
        emit sizeHintChanged();
    if (as)
        emit aspectRatioChanged();
    if (hv)
        emit hasVideoChanged(d->video);
}

auto VideoPreview::sizeHint() const -> QSize
{
    return d->displaySize;
}

auto VideoPreview::paint(OpenGLFramebufferObject *fbo) -> void
{
    fbo->bind();
    if (d->redraw) {
        d->redraw = false;
        auto w = window();
        if (w) {
            w->resetOpenGLState();
            d->mpv.render(fbo, nullptr, QMargins());
            w->resetOpenGLState();
        }
    }
    fbo->release();
}

auto VideoPreview::load(const QByteArray &path) -> void
{
    if (path.contains("bomi-yle-"_b))
        return;
    if (d->active)
        d->mpv.tellAsync("loadfile", path);
}

auto VideoPreview::unload() -> void
{
    d->mpv.tellAsync("stop");
}

auto VideoPreview::shutdown() -> void
{
    d->mpv.tellAsync("quit");
}

auto VideoPreview::setActive(bool active) -> void
{
    if (_Change(d->active, active) && !d->active)
        unload();
}

auto VideoPreview::setShowKeyframe(bool keyframe) -> void
{
    d->keyframe = keyframe;
}
