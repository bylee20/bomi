#include "videooutput.hpp"
#include "deintoption.hpp"
#include "videorendereritem.hpp"
#include "videoformat.hpp"
#include "videoframeshader.hpp"
#include "mposdbitmap.hpp"
#include "videotexture.hpp"
#include "videodata.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include "opengl/openglbenchmarker.hpp"
#include "opengl/opengllogger.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "player/playengine.hpp"
#include "player/mpv_helper.hpp"
#include "misc/log.hpp"
extern "C" {
#include <sub/osd_state.h>
#include <video/hwdec.h>
#include <video/out/vo.h>
#include <video/vfcap.h>
#include <input/input.h>
}

DECLARE_LOG_CONTEXT(Video)

#define DO_BENCHMARK 0

//static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;

auto query_video_format(quint32 format) -> int;
extern auto initialize_vdpau_interop(QOpenGLContext *context) -> void;
extern auto finalize_vdpau_interop(QOpenGLContext *context) -> void;

using VideoOsdCache = VideoImageCache<MpOsdBitmap>;

class MpOsdBitmapPool : public VideoImagePool<MpOsdBitmap> {
    using Cache = VideoOsdCache;
public:
    auto get(const sub_bitmaps *imgs, const QSize &size) -> Cache
    {
        Cache cache = this->getCache(5);
        if (!cache)
            return cache;
        auto &data = const_cast<MpOsdBitmap&>(*cache);
        data.copy(imgs, size);
        return cache;
    }
    auto clear() -> void { reserve(0); }
};

struct cmplayer_vo_priv {
    VideoOutput *vo;
    char *address;
};

static auto priv(vo *out) -> VideoOutput*
{
    return static_cast<cmplayer_vo_priv*>(out->priv)->vo;
}

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
    driver.flip_page    = VideoOutput::flipPage;
    driver.query_format = [] (vo*, quint32 f) { return query_video_format(f); };
    driver.draw_image   = VideoOutput::drawImage;
    driver.uninit       = VideoOutput::uninit;
    driver.options      = options;
    driver.priv_size    = sizeof(cmplayer_vo_priv);
    return driver;
}

vo_driver video_out_null = create_driver();

struct VideoOutput::Data {
    VideoOutput *p = nullptr;
    vo *out = nullptr;
    VideoFormat format;
    mp_image_params params;
    VideoRenderer *renderer = nullptr;

    VideoData data;
    mp_osd_res osd;
    MpOsdBitmap::Id bitmapId;
    MpOsdBitmapPool bitmapPool;
    bool hasOsd = false;

    PlayEngine *engine = nullptr;
    QPoint mouse{-1, -1};


    auto reset() -> void
    {
        memset(&osd, 0, sizeof(osd));
        bitmapPool.clear();
        bitmapId = MpOsdBitmap::Id();
        data.release();
    }

};

VideoOutput::VideoOutput(PlayEngine *engine)
    : d(new Data)
{
    d->p = this;
    d->reset();
    d->engine = engine;
//    d->marker(d->engine, &PlayEngine::deintOptionsChanged, DirtyDeint);
}

VideoOutput::~VideoOutput()
{
    delete d;
}

auto VideoOutput::setRenderer(VideoRenderer *renderer) -> void
{
    if (!_Change(d->renderer, renderer) || !d->renderer)
        return;
    d->renderer->prepare(d->format);
}

auto VideoOutput::preinit(vo *out) -> int
{
    auto priv = static_cast<cmplayer_vo_priv*>(out->priv);
    priv->vo = address_cast<VideoOutput*>(priv->address);
    Data *d = priv->vo->d;
    d->out = out;
//    initialize_vdpau_interop(d->gl->context());
    _Debug("Initialize VideoOutput");
    return 0;
}

auto VideoOutput::uninit(vo *out) -> void
{
    auto v = priv(out); Data *d = v->d;
    d->reset();
//    finalize_vdpau_interop(d->gl->context());
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
    d->params = *params;
    const auto desc = mp_imgfmt_get_desc(d->params.imgfmt);
    const auto inverted = flags & VOFLAG_FLIPPING;
    if (_Change(d->format, VideoFormat(d->params, desc, inverted))) {
        if (d->renderer)
            d->renderer->prepare(d->format);
        emit v->formatChanged(d->format);
    }
    _Debug("Configure VideoOutput with %%(%%x%%) format",
           VideoFormat::name(params->imgfmt), params->w, params->h);
    return 0;
}

auto VideoOutput::drawImage(vo *out, mp_image *mpi) -> void
{
    _Trace("VideoOutput::drawImage() with mpi=%%", mpi);
    auto v = priv(out); Data *d = v->d;
    Q_ASSERT(mp_image_params_equal(&d->params, &mpi->params));
    auto tmp = MpImage::wrap(mpi);
    d->data.setImage(tmp);
    if (!d->data.hasImage())
        return;
    static const bool format[SUBBITMAP_COUNT] = {0, 1, 1, 1};
    static auto cb = [] (void *data, struct sub_bitmaps *imgs) {
        auto d = static_cast<Data*>(data);
        if (_Change(d->bitmapId, { imgs }))
            d->data.setOsd(d->bitmapPool.get(imgs, { d->osd.w, d->osd.h }));
        d->hasOsd = true;
    };
    if (d->renderer) {
        const auto dpr = d->renderer->devicePixelRatio();
        auto size = d->renderer->osdSize();
        d->osd.w = size.width();
        d->osd.h = size.height();
        d->osd.w *= dpr;
        d->osd.h *= dpr;
        d->osd.display_par = 1.0;
        osd_draw(out->osd, d->osd, d->data.image()->pts, 0, format, cb, d);
    }
    _Trace("VideoOutput::drawImage()");
}

auto VideoOutput::flipPage(vo *out) -> void
{
    auto v = priv(out); Data *d = v->d;
    if (!d->data.hasImage() || d->format.isEmpty() || !d->renderer) {
        _Trace("VideoOutput::flipPage(): no frame to flip");
        return;
    }
    _Trace("VideoOutput::flipPage(): frame has osd: %%", d->hasOsd);
    if (!d->hasOsd)
        d->data.setOsd(VideoOsdCache());
    Q_ASSERT(!d->format.isEmpty());
    d->renderer->present(d->data);
    d->hasOsd = false;
//    d->data.setImage(MpImage{});
//    d->data.pass(nullptr);
}

auto VideoOutput::control(vo *out, uint32_t req, void *data) -> int
{
    auto v = priv(out); Data *d = v->d;
    switch (req) {
    case VOCTRL_REDRAW_FRAME: {
//        d->draw();
        return true;
    } case VOCTRL_GET_HWDEC_INFO: {
        const auto info = static_cast<mp_hwdec_info*>(data);
        info->vdpau_ctx = (mp_vdpau_ctx*)(void*)(v);
        return true;
    } case VOCTRL_RESET:
        d->reset();
        return true;
    case VOCTRL_RESUME:
        return true;
    case VOCTRL_PAUSE:
        return true;
    case VOCTRL_CHECK_EVENTS:
        if (d->renderer) {
//            Q_ASSERT(out->input_ctxopts->enable_mouse_movements);
            if (_Change(d->mouse, d->engine->mousePosition()))
                mp_input_set_mouse_pos(out->input_ctx, d->mouse.x(), d->mouse.y());
        }
//        if (d->dirty.load())
//            out->want_redraw = true;
        return true;
    case VOCTRL_GET_COLORSPACE:
        *static_cast<mp_image_params*>(data) = d->params;
        return true;
    default:
        return VO_NOTIMPL;
    }
}

auto query_video_format(quint32 format) -> int
{
    switch (format) {
    case IMGFMT_VDPAU:     case IMGFMT_VDA:       case IMGFMT_VAAPI:
    case IMGFMT_420P:      case IMGFMT_444P:
    case IMGFMT_420P16:
    case IMGFMT_420P14:
    case IMGFMT_420P12:
    case IMGFMT_420P10:
    case IMGFMT_420P9:
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
