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
    bool redraw = false, active = false;
    QSize displaySize{0, 1};
    double rate = 0.0, aspect = 4./3., percent = 0;
    Mpv mpv;
    auto vo() const -> QByteArray { return "opengl-cb"_b; }
    auto sizeAspect() const -> double
    {
        if (displaySize.isEmpty())
            return 4./3.;
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
    d->mpv.initialize();
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
    if (_Change(d->rate, rate)) {
        if (_Change(d->percent, qRound(d->rate * 10000)/100.0))
            d->mpv.tellAsync("seek", d->percent, "absolute-percent"_b, "keyframes"_b);
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

auto VideoPreview::hasFrame() const -> bool
{
    return !d->displaySize.isEmpty();
}

auto VideoPreview::setSizeHint(const QSize &size) -> void
{
    if (_Change(d->displaySize, size))
        emit sizeHintChanged();
    if (_Change(d->aspect, d->sizeAspect()))
        emit aspectRatioChanged();
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

auto VideoPreview::load(const QString &path) -> void
{
    if (d->active)
        d->mpv.tellAsync("loadfile", MpvFile(path));
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
    if (_Change(d->active, active)) {
        if (!d->active)
            unload();
    }
}
