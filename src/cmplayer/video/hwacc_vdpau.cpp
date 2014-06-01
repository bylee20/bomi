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

#define TO_INTEROP(a) (void*)(quintptr)(a)

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
    using Cache = VideoImageCache<VdpauSurface>;
    DECLARE_LOG_CONTEXT(VDPAU)
public:
    VdpauSurfacePool() = default;
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
    }
}

auto Vdpau::isAvailable() -> bool
{
    return d.init && OGL::hasExtension(OGL::NvVdpauInterop);
}

auto Vdpau::initializeInterop(QOpenGLContext *ctx) -> void
{
    if (!d.init)
        return;
    if (!d.gl) {
        d.gl = ctx;
        proc("glVDPAUInitNV"_b,                  d.initialize);
        proc("glVDPAUFiniNV"_b,                  d.finalize);
        proc("glVDPAURegisterOutputSurfaceNV"_b, d.registerOutputSurface);
        proc("glVDPAUIsSurfaceNV"_b,             d.isSurface);
        proc("glVDPAUUnregisterSurfaceNV"_b,     d.unregisterSurface);
        proc("glVDPAUGetSurfaceivNV"_b,          d.getSurfaceiv);
        proc("glVDPAUSurfaceAccessNV"_b,         d.surfaceAccess);
        proc("glVDPAUMapSurfacesNV"_b,           d.mapSurfaces);
        proc("glVDPAUUnmapSurfacesNV"_b,         d.unmapSurfaces);
    }
    OGL::logError("before glVDPAUInitNV()"_b);
    d.initialize(TO_INTEROP(d.device), TO_INTEROP(d.proc));
    OGL::logError("after glVDPAUInitNV()"_b);
}



auto Vdpau::finalizeInterop(QOpenGLContext *ctx) -> void
{
    if (d.finalize) {
        Q_ASSERT(ctx == d.gl);
        d.finalize();
    }
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
    const int width = tmp::aligned<2>(w);
    const int height = tmp::aligned<4>(h);
    if (!check(Vdpau::decoderCreate(profile.id, width, height,
                                    codec->surfaces, &d->context.decoder))) {
        d->context.decoder = VDP_INVALID_HANDLE;
        return false;
    }
    if (!d->pool.create(width, height, chroma))
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

VdpauMixer::~VdpauMixer() {
    if (m_glSurface) {
        Vdpau::unmapSurfaces(1, &m_glSurface);
        Vdpau::unregisterSurface(m_glSurface);
    }
    if (m_surface != VDP_INVALID_HANDLE)
        Vdpau::outputSurfaceDestroy(m_surface);
    if (m_mixer != VDP_INVALID_HANDLE)
        Vdpau::videoMixerDestroy(m_mixer);
}

auto VdpauMixer::create(const QList<OpenGLTexture2D> &textures) -> bool
{
    auto &texture = textures[0];
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
    m_glSurface = Vdpau::registerOutputSurface(m_surface,
                                               texture.target(), 1, &id);
    if (m_glSurface == GL_NONE
            && !check(VDP_STATUS_ERROR, "Cannot register output surface."))
        return false;
    Vdpau::surfaceAccess(m_glSurface, GL_READ_ONLY);
    Vdpau::mapSurfaces(1, &m_glSurface);
    _Debug("VDPAU Mixer initialized.");
    return true;
}

auto VdpauMixer::upload(const mp_image *mpi, bool deint) -> bool
{
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

auto VdpauMixer::getAligned(const mp_image */*mpi*/,
                            QVector<QSize> *bytes) -> mp_imgfmt
{
    const int width = tmp::aligned<2>(this->width() + 1);
    const int height = tmp::aligned<4>(this->height() + 3);
    bytes->resize(1);
    (*bytes)[0] = QSize(width*4, height);
    return IMGFMT_BGRA;
}

#endif
