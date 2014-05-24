#include "videooutput.hpp"
#include "deintoption.hpp"
#include "videorendereritem.hpp"
#include "videoformat.hpp"
#include "videoframeshader.hpp"
#include "mposdbitmap.hpp"
#include "videoframebufferobject.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include "player/playengine.hpp"
#include "player/mpv_helper.hpp"
#include "misc/log.hpp"
#include <deque>
extern "C" {
#include <sub/osd_state.h>
#include <video/hwdec.h>
#include <video/out/vo.h>
#include <video/vfcap.h>
}

static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;

auto query_video_format(quint32 format) -> int;
extern auto initialize_vdpau_interop(QOpenGLContext *context) -> void;
extern auto finalize_vdpau_interop(QOpenGLContext *context) -> void;

enum DirtyFlag {
    DirtyEffects = 1 << 0,
    DirtyKernel  = 1 << 1,
    DirtyRange   = 1 << 2,
    DirtyChroma  = 1 << 3,
    DirtySize    = 1 << 4,
    DirtyDeint   = 1 << 5
};

using MpOsdBitmapCache = VideoImageCache<MpOsdBitmap>;

class MpOsdBitmapPool : public VideoImagePool<MpOsdBitmap> {
    using Cache = MpOsdBitmapCache;
public:
    auto get(const sub_bitmaps *imgs, const QSize &size) -> Cache
    {
        Cache cache = this->getCache(3);
        if (!cache)
            return cache;
        auto &data = const_cast<MpOsdBitmap&>(*cache);
        data.copy(imgs, size);
        return cache;
    }
    auto clear() -> void { reserve(0); }
};

class VideoFramebufferObjectPool
        : public VideoImagePool<VideoFramebufferObject> {
    using Cache = VideoFramebufferObjectCache;
    OGL::TextureFormat m_fboFormat = OGL::RGBA8_UNorm;
public:
    VideoFramebufferObjectPool()
    {
        auto formats = OGL::availableFrambebufferFormats();
        if (formats.contains(OGL::RGBA16_UNorm))
            m_fboFormat = OGL::RGBA16_UNorm;
    }
    auto get(const VideoFormat &format, double pts) -> Cache
    {
        if (format.isEmpty())
            return Cache();
        Cache cache = this->getCache(3);
        if (cache.isNull())
            return cache;
        auto &data = const_cast<VideoFramebufferObject&>(cache.image());
        data.m_displaySize = format.displaySize();
        data.m_pts = pts;
        if (!data.m_fbo || data.m_fbo->size() != format.size())
            _Renew(data.m_fbo, format.size(), m_fboFormat);
        return cache;
    }
};

struct cmplayer_vo_priv {
    VideoOutput *vo;
    char *address;
};

static auto priv(vo *out) -> VideoOutput*
{
    return static_cast<cmplayer_vo_priv*>(out->priv)->vo;
}

DECLARE_LOG_CONTEXT(Video)

auto create_driver() -> vo_driver
{
#define MPV_OPTION_BASE cmplayer_vo_priv
    static m_option options[] = {
        MPV_OPTION(address),
        mpv::null_option
    };
    static vo_driver driver;
    driver.description  = "CMPlayer video output";
    driver.name         = "null";
    driver.preinit      = VideoOutput::preinit;
    driver.reconfig     = VideoOutput::reconfig;
    driver.control      = VideoOutput::control;
    driver.draw_osd     = VideoOutput::drawOsd;
    driver.flip_page    = VideoOutput::flipPage;
    driver.query_format = [] (vo*, quint32 f) { return query_video_format(f); };
    driver.draw_image   = VideoOutput::drawImage;
    driver.uninit       = VideoOutput::uninit;
    driver.options      = options;
    driver.priv_size    = sizeof(cmplayer_vo_priv);
    return driver;
}

vo_driver video_out_null = create_driver();

struct FrameTime {
    FrameTime(quint64 frames, quint64 time)
        : frames(frames), time(time) { }
    quint64 frames = 0, time = 0;
};

struct VideoOutput::Data {
    VideoOutput *p = nullptr;
    vo *out = nullptr;
    mp_image *mpi = nullptr;
    VideoFormat format;
    mp_image_params params;
    VideoRendererItem *renderer = nullptr;
    OpenGLOffscreenContext *gl = nullptr;
    VideoFrameShader *shader = nullptr;
    VideoFramebufferObjectPool *pool = nullptr;
    VideoFramebufferObjectCache cache;
    int strides[4] = {0, 0, 0, 0};

    mp_osd_res osd;
    MpOsdBitmapCache bitmap;
    MpOsdBitmapPool bitmapPool;
    bool bitmapChanged = false, hasOsd = false;

    PlayEngine *engine = nullptr;
    QAtomicInt dirty = 0;
    VideoColor color;
    QPoint mouse{-1, -1};
    quint64 drawn = 0, dropped = 0;
    std::deque<FrameTime> timings;
    double avgfps = 0.0;

    auto reset() -> void
    {
        memset(&osd, 0, sizeof(osd));
        cache = VideoFramebufferObjectCache();
        bitmap = MpOsdBitmapCache();
        bitmapPool.clear();
        mp_image_unrefp(&mpi);
    }
    auto draw() -> void
    {
        if (!mpi)
            return;
        gl->makeCurrent();
        const int dirty = this->dirty.fetchAndStoreRelaxed(0);
        if (dirty) {
            if (dirty & DirtyEffects)
                shader->setEffects(renderer->effects());
            if (dirty & DirtyRange)
                shader->setRange(engine->videoColorRange());
            if (dirty & DirtyChroma)
                shader->setChromaUpscaler(engine->videoChromaUpscaler());
            if (dirty & DirtyDeint) {
                auto method = engine->deintOptionForSwDec().method;
                if (IMGFMT_IS_HWACCEL(params.imgfmt))
                    method = engine->deintOptionForHwDec().method;
                shader->setDeintMethod(method);
            }
        }
        cache = pool->get(format, mpi->pts);
        if (!cache.isNull()) {
            shader->upload(mpi);
            cache->bind();
            shader->render(renderer->kernel());
            cache->release();
        }
        gl->doneCurrent();
        _Trace("VideoOutput::draw()");
    }
    template<class T, class... Args>
    auto marker(T *t, void(T::*sig)(Args...), DirtyFlag flag) -> void
    {
        connect(t, sig, p, [=] () { dirty.store(dirty.load() | flag); },
                Qt::DirectConnection);
    }
    void clearTimings() {
        timings.clear();
        drawn = 0;
        avgfps = 0;
    }
};

VideoOutput::VideoOutput(PlayEngine *engine)
    : d(new Data)
{
    d->p = this;
    d->reset();
    d->engine = engine;
    d->marker(d->engine, &PlayEngine::videoColorRangeChanged, DirtyRange);
    d->marker(d->engine, &PlayEngine::videoChromaUpscalerChanged, DirtyChroma);
    d->marker(d->engine, &PlayEngine::deintOptionsChanged, DirtyDeint);
}

VideoOutput::~VideoOutput()
{
    delete d;
}

auto VideoOutput::setRenderer(VideoRendererItem *renderer) -> void
{
    if (!_Change(d->renderer, renderer) || !d->renderer)
        return;
    d->marker(d->renderer, &VideoRendererItem::osdSizeChanged, DirtySize);
    d->marker(d->renderer, &VideoRendererItem::effectsChanged, DirtyEffects);
    d->marker(d->renderer, &VideoRendererItem::kernelChanged, DirtyKernel);
    d->dirty.store(0xffffffff);
}

auto VideoOutput::initializeGL(OpenGLOffscreenContext *gl) -> void
{
    Q_ASSERT(QOpenGLContext::currentContext());
    d->gl = gl;
    _Debug("Initialize OpenGL context");
}

auto VideoOutput::finalizeGL() -> void
{
    Q_ASSERT(QOpenGLContext::currentContext());
    d->gl = nullptr;
    _Debug("Finalize OpenGL context");
}

auto VideoOutput::preinit(vo *out) -> int
{
    auto priv = static_cast<cmplayer_vo_priv*>(out->priv);
    priv->vo = address_cast<VideoOutput*>(priv->address);
    Data *d = priv->vo->d;
    d->out = out;
    Q_ASSERT(d->gl);
    d->gl->makeCurrent();
    initialize_vdpau_interop(d->gl->context());
    d->shader = new VideoFrameShader;
    d->pool =new VideoFramebufferObjectPool;
    d->gl->doneCurrent();
    d->dirty.store(0xffffffff);
    _Debug("Initialize VideoOutput");
    return 0;
}

auto VideoOutput::uninit(vo *out) -> void
{
    auto v = priv(out); Data *d = v->d;
    d->gl->makeCurrent();
    d->reset();
    _Delete(d->shader);
    _Delete(d->pool);
    finalize_vdpau_interop(d->gl->context());
    d->gl->doneCurrent();
    _Debug("Uninitialize VideoOutput");
}


auto VideoOutput::format() const -> const VideoFormat&
{
    return d->format;
}

auto VideoOutput::reconfig(vo *out, mp_image_params *params, int flags) -> int
{
    auto v = priv(out); Data *d = v->d;
    d->reset();
    d->shader->setFlipped(flags & VOFLAG_FLIPPING);
    d->params = *params;
    const auto desc = mp_imgfmt_get_desc(d->params.imgfmt);
    if (_Change(d->format, VideoFormat(d->params, desc))) {
        d->gl->makeCurrent();
        d->shader->setFormat(d->params);
        d->gl->doneCurrent();
        emit v->formatChanged(d->format);
    }
    _Debug("Configure VideoOutput with %%(%%x%%) format",
           mp_imgfmt_to_name(params->imgfmt), params->w, params->h);
    return 0;
}

auto VideoOutput::droppedFrames() const -> int
{
    return d->dropped;
}

auto VideoOutput::reset() -> void
{
    if (_Change(d->dropped, 0ull))
        emit droppedFramesChanged(d->dropped);
}

auto VideoOutput::avgfps() const -> double
{
    return d->avgfps;
}

auto VideoOutput::drawImage(vo *out, mp_image *mpi) -> void
{
    _Trace("VideoOutput::drawImage() with mpi=%%", mpi);
    auto v = priv(out); Data *d = v->d;
    mp_image_setrefp(&d->mpi, mpi);
    if (!d->mpi)
        return;
    Q_ASSERT(mp_image_params_equals(&d->params, &mpi->params));
    d->draw();
    if (!d->format.isEmpty() && !d->cache)
        emit v->droppedFramesChanged(++d->dropped);
}

auto VideoOutput::drawOsd(vo *out, struct osd_state *osd) -> void
{
    static const bool format[SUBBITMAP_COUNT] = {0, 1, 1, 1};
    static auto cb = [] (void *vo, struct sub_bitmaps *imgs) {
        auto v = static_cast<VideoOutput*>(vo); auto d = v->d;
        if (!d->bitmap || d->bitmap->needToCopy(imgs)) {
            d->bitmap = d->bitmapPool.get(imgs, { d->osd.w, d->osd.h });
            d->bitmapChanged = true;
        }
        d->hasOsd = true;
    };
    auto v = priv(out); auto d = v->d;
    if (auto r = d->renderer) {
        const auto dpr = r->devicePixelRatio();
        auto size = r->osdSize();
        d->osd.w = size.width();
        d->osd.h = size.height();
        d->osd.w *= dpr;
        d->osd.h *= dpr;
        d->osd.display_par = 1.0;
        osd_draw(osd, d->osd, osd->vo_pts, 0, format, cb, v);
    }
}

auto VideoOutput::flipPage(vo *out) -> void
{
    _Trace("VideoOutput::flipPage()");
    auto v = priv(out); Data *d = v->d;
    if (!d->mpi || d->format.isEmpty() || !d->renderer)
        return;
    if (d->hasOsd)
        d->renderer->present(d->cache, d->bitmapChanged ? d->bitmap
                                                        : MpOsdBitmapCache());
    else
        d->renderer->present(d->cache);
    _Trace("Video image has been transferred to renderer");
    d->cache = VideoFramebufferObjectCache();
    d->hasOsd = d->bitmapChanged = false;
    ++d->drawn;
    constexpr int interval = 4;
    constexpr int max = 20, min = 5;
    const int size = d->timings.size();
    if (size == max && d->drawn & (interval - 1))
        return;
    d->timings.emplace_back(d->drawn, _SystemTime());
    if (size > min) {
        if (size > max)
            d->timings.pop_front();
        const auto df = d->timings.back().frames - d->timings.front().frames;
        const auto dt = d->timings.back().time - d->timings.front().time;
        d->avgfps = df/(dt*1e-6);
    } else
        d->avgfps = 0.0;
}

auto VideoOutput::control(vo *out, uint32_t req, void *data) -> int
{
    auto v = priv(out); Data *d = v->d;
    switch (req) {
    case VOCTRL_REDRAW_FRAME: {
        d->draw();
        return true;
    } case VOCTRL_GET_HWDEC_INFO: {
        const auto info = static_cast<mp_hwdec_info*>(data);
        info->vdpau_ctx = (mp_vdpau_ctx*)(void*)(v);
        return true;
    } case VOCTRL_RESET:
        d->reset();
        return true;
    case VOCTRL_WINDOW_TO_OSD_COORDS:
        return true;
    case VOCTRL_GET_EQUALIZER: {
        auto args = static_cast<voctrl_get_equalizer_args*>(data);
        const auto type = VideoColor::getType(args->name);
        if (type == VideoColor::TypeMax)
            return VO_NOTIMPL;
        *(args->valueptr) = d->color.get(type);
        return true;
    } case VOCTRL_SET_EQUALIZER: {
        auto args = static_cast<voctrl_set_equalizer_args*>(data);
        const auto type = VideoColor::getType(args->name);
        if (type == VideoColor::TypeMax)
            return VO_NOTIMPL;
        if (d->color.get(type) != args->value) {
            d->color.set(type, args->value);
            d->shader->setColor(d->color);
            out->want_redraw = true;
        }
        return true;
    } case VOCTRL_RESUME:
        d->clearTimings();
        return true;
    case VOCTRL_PAUSE:
        d->clearTimings();
        return true;
    case VOCTRL_CHECK_EVENTS:
        if (d->renderer) {
            Q_ASSERT(out->opts->enable_mouse_movements);
            if (_Change(d->mouse, d->engine->mousePosition()))
                vo_mouse_movement(out, d->mouse.x(), d->mouse.y());
        }
        if (d->dirty.load())
            out->want_redraw = true;
        return true;
    default:
        return VO_NOTIMPL;
    }
}

auto query_video_format(quint32 format) -> int
{
    switch (format) {
    case IMGFMT_VDPAU: case IMGFMT_VDA: case IMGFMT_VAAPI:
    case IMGFMT_420P:      case IMGFMT_444P:
    case IMGFMT_420P16_LE: case IMGFMT_420P16_BE:
    case IMGFMT_420P14_LE: case IMGFMT_420P14_BE:
    case IMGFMT_420P12_LE: case IMGFMT_420P12_BE:
    case IMGFMT_420P10_LE: case IMGFMT_420P10_BE:
    case IMGFMT_420P9_LE:  case IMGFMT_420P9_BE:
    case IMGFMT_NV12:      case IMGFMT_NV21:
    case IMGFMT_YUYV:      case IMGFMT_UYVY:
    case IMGFMT_BGRA:      case IMGFMT_RGBA:
    case IMGFMT_ARGB:      case IMGFMT_ABGR:
    case IMGFMT_BGR0:      case IMGFMT_RGB0:
    case IMGFMT_0RGB:      case IMGFMT_0BGR:
        return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW;
    default:
        return 0;
    }
}
