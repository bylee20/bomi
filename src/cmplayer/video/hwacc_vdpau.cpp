#include "hwacc_vdpau.hpp"
#include "opengl/opengltexture2d.hpp"
#include "tmp/static_op.hpp"

void initialize_vdpau_interop(QOpenGLContext *ctx) {
#ifdef Q_OS_LINUX
    Vdpau::initializeInterop(ctx);
#endif
    Q_UNUSED(ctx);
}

void finalize_vdpau_interop(QOpenGLContext *ctx) {
#ifdef Q_OS_LINUX
    Vdpau::finalizeInterop(ctx);
#endif
    Q_UNUSED(ctx);
}

#ifdef Q_OS_LINUX

extern "C" {
#include <libavcodec/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <vdpau/vdpau.h>
}

auto HwAccX11Trait<IMGFMT_VDPAU>::error(Status status) -> const char*
{
    if (Vdpau::isAvailable())
        return Vdpau::getErrorString(status);
    return status == success ? "SUCCESS" : "ERROR";
}

auto HwAccX11Trait<IMGFMT_VDPAU>::destroySurface(SurfaceID id) -> void
{
    if (id != invalid)
        Vdpau::videoSurfaceDestroy(id);
}

class VdpauSurfacePool : public HwAccX11SurfacePool<IMGFMT_VDPAU>
                       , public VdpauStatusChecker {
public:
    auto create(int w, int h, uint format) -> bool
    {
        if (compat(w, h, format))
            return true;
        reset(w, h, format);
        return true;
    }
    auto getMpImage() -> mp_image*
    {
        auto cache = this->getCache(50);
        if (cache.isNull()) {
            _Warn("No usable SurfaceID. Decoding could fail");
            cache = this->recycle();
        } else if (cache->m_id == VDP_INVALID_HANDLE) {
            auto &surface = const_cast<VdpauSurface&>(*cache);
            if (!check(Vdpau::videoSurfaceCreate(format(), width(), height(),
                                                 &surface.m_id),
                       "Cannot create VDPAU surface")) {
                surface.m_id = VDP_INVALID_HANDLE;
                return nullptr;
            }
            surface.m_format = format();
        }
        return wrap(new Cache(std::move(cache)));
    }
};

Vdpau::Data Vdpau::d;

auto Vdpau::initialize() -> void
{
    if (d.init || !OGL::hasExtension(OGL::NvVdpauInterop))
        return;
    if (!d.check(vdp_device_create_x11(QX11Info::display(),
                                       QX11Info::appScreen(),
                                       &d.device, &d.proc),
                 "Cannot intialize VDPAU device"))
        return;
#define PROC(id, f) proc(VDP_FUNC_ID_##id, f)
    PROC(GET_ERROR_STRING,                 d.getErrorString);
    PROC(GET_INFORMATION_STRING,           d.getInformationString);
    PROC(DEVICE_DESTROY,                   d.deviceDestroy);
    PROC(VIDEO_SURFACE_QUERY_CAPABILITIES, d.videoSurfaceQueryCaps);
    PROC(VIDEO_SURFACE_CREATE,             d.videoSurfaceCreate);
    PROC(VIDEO_SURFACE_DESTROY,            d.videoSurfaceDestroy);
    PROC(VIDEO_SURFACE_GET_BITS_Y_CB_CR,   d.videoSurfaceGetBitsYCbCr);
    PROC(DECODER_CREATE,                   d.decoderCreate);
    PROC(DECODER_DESTROY,                  d.decoderDestroy);
    PROC(DECODER_RENDER,                   d.decoderRender);
    PROC(DECODER_QUERY_CAPABILITIES,       d.decoderQueryCaps);
    PROC(VIDEO_MIXER_CREATE,               d.videoMixerCreate);
    PROC(VIDEO_MIXER_DESTROY,              d.videoMixerDestroy);
    PROC(VIDEO_MIXER_RENDER,               d.videoMixerRender);
    PROC(OUTPUT_SURFACE_CREATE,            d.outputSurfaceCreate);
    PROC(OUTPUT_SURFACE_DESTROY,           d.outputSurfaceDestroy);
#undef PROC
    if (!d.check(d.status(), "Cannot get VDPAU functions."))
        return;
    auto get = [] (VdpDecoderProfile id) -> VdpauProfile
    {
        VdpBool supported = false;
        quint32 lv = 0, blocks = 0, w = 0, h = 0;
        VdpauProfile profile;
        if (d.decoderQueryCaps(d.device, id, &supported, &lv, &blocks,
                                       &w, &h) == VDP_STATUS_OK && supported) {
            profile.id = id; profile.level = lv; profile.blocks = blocks;
            profile.width = w; profile.height = h;
        }
        return profile;
    };

    auto push = [] (const QVector<VdpauCodec::ProfilePair> &pairs,
                    AVCodecID codec, int surfaces)
    {
        for (auto &pair : pairs) {
            if (pair.hw.id == VDP_DECODER_PROFILE_NONE)
                continue;
            auto &supports = d.supports[codec];
            supports.id = codec;
            supports.profiles.push_back(pair);
            Q_ASSERT(surfaces <= 16);
            supports.surfaces = surfaces;
        }
    };
#define PAIR(vdp, ff) { get(VDP_DECODER_PROFILE_##vdp), FF_PROFILE_##ff }
    const QVector<VdpauCodec::ProfilePair> mpeg1s = {
        PAIR(MPEG1, UNKNOWN)
    };
    const QVector<VdpauCodec::ProfilePair> mpeg2s = {
        PAIR(MPEG2_MAIN,   MPEG2_MAIN),
        PAIR(MPEG2_SIMPLE, MPEG2_SIMPLE)
    };
    const QVector<VdpauCodec::ProfilePair> h264s = {
        PAIR(H264_HIGH,     H264_HIGH),
        PAIR(H264_BASELINE, H264_BASELINE),
        PAIR(H264_BASELINE, H264_CONSTRAINED_BASELINE),
        PAIR(H264_MAIN,     H264_MAIN)
    };
    const QVector<VdpauCodec::ProfilePair> vc1s = {
        PAIR(VC1_ADVANCED, VC1_ADVANCED),
        PAIR(VC1_SIMPLE,   VC1_SIMPLE),
        PAIR(VC1_MAIN,     VC1_MAIN)
    };
    const QVector<VdpauCodec::ProfilePair> wmv3s = {
        PAIR(VC1_MAIN,     VC1_MAIN),
        PAIR(VC1_SIMPLE,   VC1_SIMPLE),
        PAIR(VC1_ADVANCED, VC1_ADVANCED)
    };
    const QVector<VdpauCodec::ProfilePair> mpeg4s = {
        PAIR(MPEG4_PART2_ASP, MPEG4_ADVANCED_SIMPLE),
        PAIR(MPEG4_PART2_SP,  MPEG4_SIMPLE)
    };
#undef PAIR
    push(mpeg1s, AV_CODEC_ID_MPEG1VIDEO, 2);
    push(mpeg2s, AV_CODEC_ID_MPEG2VIDEO, 2);
    push(h264s, AV_CODEC_ID_H264, 16);
    push(vc1s, AV_CODEC_ID_VC1, 2);
    push(wmv3s, AV_CODEC_ID_WMV3, 2);
    push(mpeg4s, AV_CODEC_ID_MPEG4, 2);
    _Debug("VDPAU device is initialized.");
    d.init = true;
}

auto Vdpau::finalize() -> void
{
    if (d.init) {
        deviceDestroy();
        d.init = false;
        _Debug("VDPAU device is finalized.");
    }
}

auto Vdpau::isAvailable() -> bool
{
    return d.init && OGL::hasExtension(OGL::NvVdpauInterop);
}

QHash<QOpenGLContext*, Vdpau::Interop> s_interops;

auto Vdpau::initializeInterop(QOpenGLContext *ctx) -> void
{
    if (!d.init)
        return;
    if (!s_interops.contains(ctx)) {
        auto &d = s_interops[ctx];
        d.gl = ctx;
        d.proc("glVDPAUInitNV"_b,                  d.initialize);
        d.proc("glVDPAUFiniNV"_b,                  d.finalize);
        d.proc("glVDPAURegisterOutputSurfaceNV"_b, d._registerOutputSurface);
        d.proc("glVDPAUIsSurfaceNV"_b,             d.isSurface);
        d.proc("glVDPAUUnregisterSurfaceNV"_b,     d.unregisterSurface);
        d.proc("glVDPAUGetSurfaceivNV"_b,          d.getSurfaceiv);
        d.proc("glVDPAUSurfaceAccessNV"_b,         d.surfaceAccess);
        d.proc("glVDPAUMapSurfacesNV"_b,           d.mapSurfaces);
        d.proc("glVDPAUUnmapSurfacesNV"_b,         d.unmapSurfaces);
        OGL::logError("before glVDPAUInitNV()"_b);
        d.initialize(TO_INTEROP(Vdpau::d.device), TO_INTEROP(Vdpau::d.proc));
        OGL::logError("after glVDPAUInitNV()"_b);
    }
}

auto Vdpau::finalizeInterop(QOpenGLContext *ctx) -> void
{
    auto it = s_interops.find(ctx);
    if (it != s_interops.end()) {
        Q_ASSERT(it->gl == ctx);
        it->finalize();
        s_interops.erase(it);
    }
}

auto Vdpau::interop() -> const Interop*
{
    auto it = s_interops.find(QOpenGLContext::currentContext());
    return it != s_interops.end() ? &(*it) : nullptr;
}

struct VdpauVideoSurface {
    VdpVideoSurface id = VDP_INVALID_HANDLE;
    bool ref = false;
};

struct HwAccVdpau::Data {
    AVVDPAUContext context;
    VdpauSurfacePool pool;
};

HwAccVdpau::HwAccVdpau(AVCodecID cid)
: HwAcc(cid), d(new Data) {
    memset(&d->context, 0, sizeof(d->context));
    d->context.decoder = VDP_INVALID_HANDLE;
    d->context.render = Vdpau::decoderRender;
}

HwAccVdpau::~HwAccVdpau() {
    freeContext();
    delete d;
}

auto HwAccVdpau::freeContext() -> void
{
    if (d->context.decoder != VDP_INVALID_HANDLE) {
        Vdpau::decoderDestroy(d->context.decoder);
        d->context.decoder = VDP_INVALID_HANDLE;
    }
}

auto HwAccVdpau::fillContext(AVCodecContext *avctx, int w, int h) -> bool
{
    if (!isSuccess())
        return false;
    freeContext();
    auto codec = Vdpau::codec(avctx->codec_id);
    if (!codec)
        return false;
    const auto profile = codec->profile(avctx->profile);
    if (profile.width < w || profile.height < h || profile.level < avctx->level)
        return false;
    VdpBool supports = false; uint mw = 0, mh = 0;
    constexpr VdpChromaType chroma = VDP_CHROMA_TYPE_420;
    if (!check(Vdpau::videoSurfaceQueryCaps(chroma, &supports, &mw, &mh)))
        return false;
    if (!supports || (int)mw < w || (int)mh < h)
        return false;
    if (!check(Vdpau::decoderCreate(profile.id, w, h, codec->surfaces,
                                    &d->context.decoder))) {
        d->context.decoder = VDP_INVALID_HANDLE;
        return false;
    }
    if (!d->pool.create(w, h, chroma))
        return false;
    return true;
}

auto HwAccVdpau::getSurface() -> mp_image*
{
    return d->pool.getMpImage();
}

auto HwAccVdpau::context() const -> void*
{
    return &d->context;
}

auto HwAccVdpau::getImage(mp_image *mpi) -> mp_image*
{
    return mpi;
}

/******************************************************************************/

VdpauMixer::VdpauMixer(const QSize &size)
: HwAccMixer(size) {

}

VdpauMixer::~VdpauMixer()
{
    release();
}

auto VdpauMixer::release() -> void
{
    auto interop = Vdpau::interop();
    Q_ASSERT(interop);
    if (m_glSurface) {
        interop->unmapSurfaces(1, &m_glSurface);
        interop->unregisterSurface(m_glSurface);
        m_glSurface = GL_NONE;
    }
    if (m_surface != VDP_INVALID_HANDLE) {
        Vdpau::outputSurfaceDestroy(m_surface);
        m_surface = VDP_INVALID_HANDLE;
    }
    if (m_mixer != VDP_INVALID_HANDLE) {
        Vdpau::videoMixerDestroy(m_mixer);
        m_mixer = VDP_INVALID_HANDLE;
    }
}

auto VdpauMixer::create(const OpenGLTexture2D &texture) -> bool
{
    auto interop = Vdpau::interop();
    texture.bind();
    static const QVector<VdpVideoMixerParameter> params = {
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
        VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
    };
    const quint32 width = this->width(), height = this->height();
    const QVector<const void *> values = { &width, &height, &m_chroma };
    if (!check(Vdpau::videoMixerCreate(0, nullptr, params.size(),
                                       params.data(), values.data(), &m_mixer),
               "Cannot create video mixer."))
        return false;
    if (!check(Vdpau::outputSurfaceCreate(VDP_RGBA_FORMAT_B8G8R8A8,
                                          width, height, &m_surface),
               "Cannot create output surface."))
        return false;
    const auto id = texture.id();
    m_glSurface = interop->registerOutputSurface(m_surface, texture.target(), 1, &id);
    if (m_glSurface == GL_NONE
            && !check(VDP_STATUS_ERROR, "Cannot register output surface."))
        return false;
    interop->surfaceAccess(m_glSurface, GL_READ_ONLY);
    interop->mapSurfaces(1, &m_glSurface);
    _Debug("VDPAU Mixer initialized.");
    return true;
}

auto VdpauMixer::upload(OpenGLTexture2D &texture,
                        const mp_image *mpi, bool deint) -> bool
{
    if (m_id != texture.id()) {
        release();
        if (!create(texture))
            return false;
        m_id = texture.id();
    }
    auto structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
    if (deint) {
        if (mpi->fields & MP_IMGFIELD_TOP)
            structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
        else if (mpi->fields & MP_IMGFIELD_BOTTOM)
            structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    }
    const auto id = (VdpVideoSurface)(quintptr)(mpi->planes[3]);
    return check(Vdpau::videoMixerRender(m_mixer, VDP_INVALID_HANDLE, nullptr,
                                         structure, 0, nullptr, id, 0, nullptr,
                                         nullptr, m_surface, nullptr, nullptr,
                                         0, nullptr),
                 "Cannot render video surface.");
}

#endif
