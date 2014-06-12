#include "hwacc_vaapi.hpp"
#include "opengl/opengltexture2d.hpp"
#include "tmp/static_op.hpp"

#ifdef Q_OS_LINUX

extern "C" {
#include <common/av_common.h>
#include <va/va_glx.h>
#include <video/sws_utils.h>
#include <libavcodec/vaapi.h>
}

bool VaApi::init = false;
bool VaApi::ok = false;
VADisplay VaApi::m_display = nullptr;
VaApi &VaApi::get() {static VaApi info; return info;}

auto HwAccX11Trait<IMGFMT_VAAPI>::destroySurface(SurfaceID id) -> void
{
   vaDestroySurfaces(VaApi::glx(), &id, 1);
}

class VaApiSurfacePool : public HwAccX11SurfacePool<IMGFMT_VAAPI>
                       , public VaApiStatusChecker {
public:
    VaApiSurfacePool() = default;
    auto create(int size, int w, int h, uint format) -> bool
    {
        if (compat(w, h, format) && this->count() == size)
            return true;
        reset(w, h, format);
        m_ids.resize(size);
        if (!isSuccess(vaCreateSurfaces(VaApi::glx(), w, h, format,
                                        m_ids.size(), m_ids.data()))) {
            _Error("Cannot create hwacc surfaces. Decoding will fail.");
            m_ids.clear();
            return false;
        }
        int i = 0;
        this->reserve(size, [&] (VaApiSurface &surface) {
            surface.m_id = m_ids[i++];
            surface.m_format = format;
        });
        return true;
    }
    auto getMpImage() -> mp_image*
    {
        auto cache = this->getUnusedCache();
        if (cache.isNull()) {
            _Warn("No usable SurfaceID. Decoding could fail");
            cache = this->recycle();
        }
        return wrap(new Cache(std::move(cache)));
    }
    auto ids() const -> const QVector<VASurfaceID> &{return m_ids;}
private:
    QVector<VASurfaceID> m_ids;
};


#ifdef USE_VAVPP
auto VaApi::toVAType(DeintMethod method) -> VAProcDeinterlacingType
{
    switch (method) {
    case DeintMethod::Bob:
        return VAProcDeinterlacingBob;
    case DeintMethod::MotionAdaptive:
        return VAProcDeinterlacingMotionAdaptive;
    default:
        return VAProcDeinterlacingNone;
    }
}

auto VaApiFilterInfo::description(VAProcFilterType type,
                                  int algorithm) -> QString
{
    switch (type) {
    case VAProcFilterNoiseReduction:
        return u"Noise reduction filter"_q;
    case VAProcFilterSharpening:
        return u"Sharpening filter"_q;
    case VAProcFilterDeinterlacing:
        switch (algorithm) {
        case VAProcDeinterlacingBob:
            return u"Bob deinterlacer"_q;
        case VAProcDeinterlacingWeave:
            return u"Weave deinterlacer"_q;
        case VAProcDeinterlacingMotionAdaptive:
            return u"Motion adaptive deinterlacer"_q;
        case VAProcDeinterlacingMotionCompensated:
            return u"Motion compensation deinterlacer"_q;
        default:
            break;
        }
    default:
        break;
    }
    return "";
}

auto VaApi::algorithms(VAProcFilterType type) -> QList<int>
{
    QList<int> ret;
    switch (type) {
    case VAProcFilterNoiseReduction:
    case VAProcFilterSharpening:
        ret << type;
        break;
    case VAProcFilterDeinterlacing:
        for (int i=1; i<VAProcDeinterlacingCount; ++i)
            ret << i;
        break;
    default:
        break;
    }
    return ret;
}

VaApiFilterInfo::VaApiFilterInfo(VAContextID context, VAProcFilterType type)
{
    m_type = type;
    uint size = 0;
    auto dpy = VaApi::glx();
    switch (type) {
    case VAProcFilterNoiseReduction:
    case VAProcFilterSharpening: {
        VAProcFilterCap cap; size = 1;
        if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, type, &cap, &size)) || size != 1)
            return;
        m_caps.resize(1); m_caps[0].algorithm = type; m_caps[0].range = cap.range;
        break;
    } case VAProcFilterDeinterlacing: {
        size = VAProcDeinterlacingCount;
        VAProcFilterCapDeinterlacing caps[VAProcDeinterlacingCount];
        if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, VAProcFilterDeinterlacing, caps, &size)))
            return;
        m_caps.resize(size);
        for (uint i=0; i<size; ++i)
            m_caps[i].algorithm = caps[i].type;
        break;
    } case VAProcFilterColorBalance: {
        size = VAProcColorBalanceCount;
        VAProcFilterCapColorBalance caps[VAProcColorBalanceCount];
        if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, VAProcFilterColorBalance, caps, &size)))
            return;
        m_caps.resize(size);
        for (uint i=0; i<size; ++i) {
            m_caps[i].algorithm = caps[i].type;
            m_caps[i].range = caps[i].range;
        }
        break;
    } default:
        return;
    }
    m_algorithms.resize(m_caps.size());
    for (int i=0; i<m_caps.size(); ++i)
        m_algorithms[i] = m_caps[i].algorithm;
}
#endif

VaApi::VaApi() {
    init = true;
    auto xdpy = QX11Info::display();
    VADisplay display = m_display = vaGetDisplayGLX(xdpy);
    if (!check(display ? VA_STATUS_SUCCESS : VA_STATUS_ERROR_UNIMPLEMENTED,
               "Cannot create VADisplay."))
        return;
    int major, minor;
    if (!check(vaInitialize(m_display, &major, &minor),
               "Cannot initialize VA-API."))
        return;
    auto size = vaMaxNumProfiles(display);
    m_profiles.resize(size);
    if (!check(vaQueryConfigProfiles(display, m_profiles.data(), &size),
               "No available profiles."))
        return;
    m_profiles.resize(size);

    for (auto profile : m_profiles) {
        int size = vaMaxNumEntrypoints(display);
        QVector<VAEntrypoint> entries(size);
        if (!isSuccess(vaQueryConfigEntrypoints(display, profile,
                                                entries.data(), &size)))
            continue;
        entries.resize(size);
        m_entries.insert(profile, entries);
    }

    initCodecs();
#ifdef USE_VAVPP
    initFilters();
#endif
    _Debug("VA-API is initialized.");
    ok = true;
}

auto VaApi::initialize() -> void
{
    if (!VaApi::init)
        VaApi::get().glx();
}

auto VaApi::finalize() -> void
{
    if (!VaApi::init)
        return;
    auto &dpy = VaApi::get().m_display;
    if (dpy) {
        vaTerminate(dpy);
        dpy = nullptr;
    }
    init = false;
}

auto VaApi::initCodecs() -> void
{
    auto supports = [this](const QVector<VaApiCodec::ProfilePair> &pairs,
                           int surfaces, AVCodecID id)
    {
        QVector<VaApiCodec::ProfilePair> list;
        for (auto &pair : pairs) {
            if (hasEntryPoint(VAEntrypointVLD, pair.hw))
                list.push_back(pair);
        }
        if (!list.isEmpty())
            m_supported[id] = VaApiCodec(m_profiles, list, surfaces + 2, id);
    };
#define PAIR(va, ff) { VAProfile##va,   FF_PROFILE_##ff }
    const QVector<VaApiCodec::ProfilePair> mpeg2s = {
        PAIR(MPEG2Main,   MPEG2_MAIN),
        PAIR(MPEG2Simple, MPEG2_SIMPLE)
    };
    const QVector<VaApiCodec::ProfilePair> mpeg4s = {
        PAIR(MPEG4Main,           MPEG4_MAIN),
        PAIR(MPEG4AdvancedSimple, MPEG4_ADVANCED_SIMPLE),
        PAIR(MPEG4Simple,         MPEG4_SIMPLE)
    };
    const QVector<VaApiCodec::ProfilePair> h264s = {
        PAIR(H264High,                H264_HIGH),
        PAIR(H264Main,                H264_MAIN),
        PAIR(H264Baseline,            H264_BASELINE),
        PAIR(H264ConstrainedBaseline, H264_CONSTRAINED_BASELINE)
    };
    const QVector<VaApiCodec::ProfilePair> wmv3s = {
        PAIR(VC1Main,     VC1_MAIN),
        PAIR(VC1Simple,   VC1_SIMPLE),
        PAIR(VC1Advanced, VC1_ADVANCED)
    };
    const QVector<VaApiCodec::ProfilePair> vc1s = {
        PAIR(VC1Advanced, VC1_ADVANCED),
        PAIR(VC1Main,     VC1_MAIN),
        PAIR(VC1Simple,   VC1_SIMPLE)
    };
#undef PAIR
    supports(mpeg2s, 3, AV_CODEC_ID_MPEG1VIDEO);
    supports(mpeg2s, 3, AV_CODEC_ID_MPEG2VIDEO);
    supports(mpeg4s, 3, AV_CODEC_ID_MPEG4);
    supports(wmv3s,  3, AV_CODEC_ID_WMV3);
    supports(vc1s,   3, AV_CODEC_ID_VC1);
    supports(h264s, 16, AV_CODEC_ID_H264);
}

#ifdef USE_VAVPP
auto VaApi::initFilters() -> void
{
    if (!hasEntryPoint(VAEntrypointVideoProc, VAProfileNone))
        return;
    auto display = VaApi::glx();
    VAConfigID config = VA_INVALID_ID;
    VAContextID context = VA_INVALID_ID;
    do {
        if (!isSuccess(vaCreateConfig(display, VAProfileNone,
                                      VAEntrypointVideoProc, nullptr, 0, &config)))
            break;
        if (!isSuccess(vaCreateContext(display, config, 0, 0, 0, nullptr, 0, &context)))
            break;
        QVector<VAProcFilterType> types(VAProcFilterCount);
        uint size = VAProcFilterCount;
        if (!isSuccess(vaQueryVideoProcFilters(display, context, types.data(), &size)))
            break;
        types.resize(size);
        for (const auto &type : types) {
            VaApiFilterInfo info(context, type);
            if (info.isSuccess() && !info.algorithms().isEmpty())
                m_filters.insert(type, info);
        }
    } while (false);
    if (context != VA_INVALID_ID)
        vaDestroyContext(display, context);
    if (config != VA_INVALID_ID)
        vaDestroyConfig(display, config);
}
#endif

auto VaApi::toVAType(int mp_fields, bool first) -> int
{
    static const int field[] = {VA_BOTTOM_FIELD, VA_TOP_FIELD};
    return field[(!(mp_fields & MP_IMGFIELD_TOP_FIRST)) ^ (int)first];
}

/******************************************************************************/

struct HwAccVaApi::Data {
    vaapi_context context;
    VAProfile profile = VAProfileNone;
    VaApiSurfacePool pool;
};

HwAccVaApi::HwAccVaApi(AVCodecID cid)
: HwAcc(cid), d(new Data) {
    memset(&d->context, 0, sizeof(d->context));
    d->context.config_id = d->context.context_id = VA_INVALID_ID;
}

HwAccVaApi::~HwAccVaApi() {
    freeContext();
    if (d->context.config_id != VA_INVALID_ID)
        vaDestroyConfig(d->context.display, d->context.config_id);
    delete d;
}

auto HwAccVaApi::isOk() const -> bool
{
    return status() == VA_STATUS_SUCCESS;
}

auto HwAccVaApi::context() const -> void*
{
    return &d->context;
}

auto HwAccVaApi::getSurface() -> mp_image*
{
    return d->pool.getMpImage();
}

auto HwAccVaApi::freeContext() -> void
{
    if (d->context.display) {
        if (d->context.context_id != VA_INVALID_ID)
            vaDestroyContext(d->context.display, d->context.context_id);
    }
    d->context.context_id = VA_INVALID_ID;
}

auto HwAccVaApi::fillContext(AVCodecContext *avctx, int w, int h) -> bool
{
    if (status() != VA_STATUS_SUCCESS)
        return false;
    freeContext();
    d->context.display = VaApi::glx();
    if (!d->context.display)
        return false;
    const auto codec = VaApi::codec(avctx->codec_id);
    if (!codec)
        return false;
    d->profile = codec->profile(avctx->profile);
    VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
    if(!isSuccess(vaGetConfigAttributes(d->context.display,
                                        d->profile, VAEntrypointVLD, &attr, 1)))
        return false;
    const uint rts =  attr.value & (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV422);
    if(!rts)
        return isSuccess(VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT);
    if(!isSuccess(vaCreateConfig(d->context.display, d->profile,
                                 VAEntrypointVLD, &attr, 1,
                                 &d->context.config_id)))
        return false;
    auto tryRtFormat = [rts, this, codec, w, h] (uint rt)
        { return (rts & rt) && d->pool.create(codec->surfaces, w, h, rt); };
    if (!tryRtFormat(VA_RT_FORMAT_YUV420) && !tryRtFormat(VA_RT_FORMAT_YUV422))
        return false;
    auto ids = d->pool.ids();
    if (!isSuccess(vaCreateContext(d->context.display, d->context.config_id,
                                   w, h, VA_PROGRESSIVE, ids.data(), ids.size(),
                                   &d->context.context_id)))
        return false;
    return true;
}

auto HwAccVaApi::getImage(mp_image *mpi) -> mp_image*
{
    return mpi;
}

/******************************************************************************/

VaApiMixer::VaApiMixer(const QSize &size)
    : HwAccMixer(size)
{

}

VaApiMixer::~VaApiMixer()
{
    release();
}

auto VaApiMixer::release() -> void
{
    if (m_glSurface) {
        vaDestroySurfaceGLX(VaApi::glx(), m_glSurface);
        m_glSurface = nullptr;
    }
}

auto VaApiMixer::upload(OpenGLTexture2D &texture,
                        const mp_image *mpi, bool deint) -> bool
{
    if (m_id != texture.id()) {
        release();
        texture.bind();
        if (!check(vaCreateSurfaceGLX(VaApi::glx(), texture.target(),
                                      texture.id(), &m_glSurface),
                   "Cannot create OpenGL surface."))
            return false;
        m_id = texture.id();
    }
    if (!m_glSurface)
        return false;
    static const int specs[MP_CSP_COUNT] = {
        0,                    //MP_CSP_AUTO,
        VA_SRC_BT601,        //MP_CSP_BT_601,
        VA_SRC_BT709,        //MP_CSP_BT_709,
        VA_SRC_SMPTE_240,    //MP_CSP_SMPTE_240M,
        0,                    //MP_CSP_RGB,
        0,                    //MP_CSP_XYZ,
        0,                    //MP_CSP_YCGCO,
    };
    const auto id = (VASurfaceID)(quintptr)mpi->planes[3];
    int flags = specs[mpi->params.colorspace];
    if (deint) {
        if (mpi->fields & MP_IMGFIELD_TOP)
            flags |= VA_TOP_FIELD;
        else if (mpi->fields & MP_IMGFIELD_BOTTOM)
            flags |= VA_BOTTOM_FIELD;
    }
    if (!check(vaCopySurfaceGLX(VaApi::glx(), m_glSurface, id,  flags),
               "Cannot copy OpenGL surface."))
        return false;
    if (!check(vaSyncSurface(VaApi::glx(), id), "Cannot sync video surface."))
        return false;
    return true;
}

#endif
