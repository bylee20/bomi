#include "hwacc.hpp"
#include "videoformat.hpp"
#include "hwacc_vaapi.hpp"
#include "hwacc_vdpau.hpp"
#include "hwacc_vda.hpp"
#include "enum/deintmethod.hpp"
extern "C" {
#undef bswap_16
#undef bswap_32
#include <video/decode/lavc.h>
#include <common/av_common.h>
}

auto HwAcc::availableBackends() -> QList<HwAcc::Type>
{
    QList<Type> list;
#ifdef Q_OS_MAC
    list.append(Vda);
#endif
#ifdef Q_OS_LINUX
    if (VaApi::isAvailable())
        list.append(VaApiGLX);
    if (Vdpau::isAvailable())
        list.append(VdpauX11);
#endif
    return list;
}

auto HwAcc::backend(const QString &name) -> HwAcc::Type
{
    if (name == "vaapi"_a)
        return VaApiGLX;
    if (name == "vdpau"_a)
        return VdpauX11;
    if (name == "vda"_a)
        return Vda;
#undef None
    return None;
}

auto HwAcc::backendName(Type type) -> QString
{
    switch (type) {
    case Vda:
        return u"vda"_q;
    case VaApiGLX:
        return u"vaapi"_q;
    case VdpauX11:
        return u"vdpau"_q;
    default:
        return QString();
    }
}

auto HwAcc::backendDescription(Type type) -> QString
{
    switch (type) {
    case Vda:
        return u"VDA(Video Decode Acceleration"_q;
    case VaApiGLX:
        return u"VA-API(Video Acceleration API)"_q;
    case VdpauX11:
        return u"VDPAU(Video Decode and Presentation API for Unix)"_q;
    default:
        return QString();
    }
}

auto HwAcc::supports(Type backend, AVCodecID codec) -> bool
{
    switch (backend) {
    case Vda:
        return codec == AV_CODEC_ID_H264;
#ifdef Q_OS_LINUX
    case VdpauX11:
        return Vdpau::codec(codec) != nullptr;
    case VaApiGLX:
        return VaApi::codec(codec) != nullptr;
#endif
    default:
        return false;
    }
}

auto HwAcc::supports(DeintMethod method) -> bool
{
#ifdef Q_OS_MAC
    Q_UNUSED(method);
    return false;
#endif
#ifdef Q_OS_LINUX
#ifdef USE_VAVPP
    auto filter = VaApi::filter(VAProcFilterDeinterlacing);
    return filter && filter->supports(VaApi::toVAType(method));
#else
    return method == DeintMethod::Bob;
#endif
#endif
}

auto HwAcc::fullDeintList() -> QList<DeintMethod>
{
    static auto list = QList<DeintMethod>() << DeintMethod::Bob;
    return list;
}

auto HwAcc::initialize() -> void
{
#ifdef Q_OS_LINUX
    Vdpau::initialize();
    VaApi::initialize();
#endif
}

auto HwAcc::finalize() -> void
{
#ifdef Q_OS_LINUX
    Vdpau::finalize();
    VaApi::finalize();
#endif
}

auto HwAcc::createMixer(mp_imgfmt imgfmt, const QSize &size) -> HwAccMixer*
{
    switch (imgfmt) {
#ifdef Q_OS_LINUX
    case IMGFMT_VDPAU:
        return new VdpauMixer(size);
    case IMGFMT_VAAPI:
        return new VaApiMixer(size);
#endif
#ifdef Q_OS_MAC
    case IMGFMT_VDA:
        return new VdaMixer(textures, format);
#endif
    default:
        return nullptr;
    }
}

struct CodecInfo {
    CodecInfo(AVCodecID id = AV_CODEC_ID_NONE,
              const char *name = "unknown")
        : id(id), name(name) {}
    AVCodecID id; const char *name;
};

static const CodecInfo codecs[] = {
    {AV_CODEC_ID_MPEG1VIDEO, "mpeg1video"},
    {AV_CODEC_ID_MPEG2VIDEO, "mpeg2video"},
    {AV_CODEC_ID_MPEG4,      "mpeg4"},
    {AV_CODEC_ID_WMV3,       "wmv3"},
    {AV_CODEC_ID_VC1,        "vc1"},
    {AV_CODEC_ID_H264,       "h264"}
};

auto HwAcc::codecName(int id) -> const char*
{
    for (auto &info : codecs) {
        if (info.id == id)
            return info.name;
    }
    return "unknow";
}

auto HwAcc::codecId(const char *name) -> AVCodecID
{
    if (!name)
        return AV_CODEC_ID_NONE;
    for (auto &info : codecs) {
        if (qstrcmp(info.name, name) == 0)
            return info.id;
    }
    return AV_CODEC_ID_NONE;
}

auto HwAcc::fullCodecList() -> QList<AVCodecID>
{
    static QList<AVCodecID> list;
    if (list.isEmpty()) {
        for (auto &info : codecs)
            list << info.id;
    }
     return list;
}

struct HwAcc::Data {
    mp_image nullImage;
    int imgfmt = IMGFMT_NONE;
};

HwAcc::HwAcc(AVCodecID codec)
: d(new Data), m_codec(codec) {
    memset(&d->nullImage, 0, sizeof(d->nullImage));
}

HwAcc::~HwAcc() {
    delete d;
}

auto HwAcc::vo(lavc_ctx *ctx) -> VideoOutput*
{
    return static_cast<VideoOutput*>((void*)(ctx->hwdec_info->vdpau_ctx));
}

auto HwAcc::imgfmt() const -> int
{
    return d->imgfmt;
}

auto HwAcc::init(lavc_ctx *ctx) -> int
{
    auto format = ctx->hwdec->image_format;
    if (!format)
        return -1;
    HwAcc *acc = nullptr;
#ifdef Q_OS_LINUX
    if (format == IMGFMT_VAAPI)
        acc = new HwAccVaApi(ctx->avctx->codec_id);
    else if (format == IMGFMT_VDPAU)
        acc = new HwAccVdpau(ctx->avctx->codec_id);
#endif
#ifdef Q_OS_MAC
    if (format == IMGFMT_VDA)
        acc = new HwAccVda(ctx->avctx->codec_id);
#endif
    if (!acc || !acc->isOk()) {
        delete acc;
        return -1;
    }
    acc->d->imgfmt = format;
    ctx->hwdec_priv = acc;
    ctx->avctx->hwaccel_context = acc->context();
    return 0;
}

auto HwAcc::uninit(lavc_ctx *ctx) -> void
{
    delete static_cast<HwAcc*>(ctx->hwdec_priv);
    ctx->hwdec_priv = nullptr;
}

auto HwAcc::initDecoder(lavc_ctx *ctx, int imgfmt, int w, int h) -> int
{
    auto acc = static_cast<HwAcc*>(ctx->hwdec_priv);
    if (imgfmt != acc->d->imgfmt || !acc->isOk())
        return -1;
    if (!acc->fillContext(ctx->avctx, w, h))
        return -1;
    acc->m_size = QSize(w, h);
    return 0;
}

auto HwAcc::allocateImage(lavc_ctx *ctx, int imgfmt, int w, int h) -> mp_image*
{
    auto acc = static_cast<HwAcc*>(ctx->hwdec_priv);
    if (imgfmt != acc->d->imgfmt || !acc->isOk() || acc->m_size != QSize(w, h))
        return nullptr;
    return acc->getSurface();
}

auto HwAcc::probe(vd_lavc_hwdec *hwdec, mp_hwdec_info *info,
                  const char *decoder) -> int
{
    if (!info || !info->vdpau_ctx)
        return HWDEC_ERR_NO_CTX;
    auto conv = [](hwdec_type mptype) {
        switch (mptype) {
        case HWDEC_VAAPI: return HwAcc::VaApiGLX;
        case HWDEC_VDPAU: return HwAcc::VdpauX11;
        case HWDEC_VDA:   return HwAcc::Vda;
        default:          return HwAcc::None;
        }
    };
    const auto codec = static_cast<AVCodecID>(mp_codec_to_av_codec_id(decoder));
    if (supports(conv(hwdec->type), codec))
        return 0;
    return HWDEC_ERR_NO_CODEC;
}

auto create_lavc_hwdec(hwdec_type type, mp_imgfmt imgfmt) -> vd_lavc_hwdec
{
    vd_lavc_hwdec hwdec;
    memset(&hwdec, 0, sizeof(hwdec));
    hwdec.type = type;
    hwdec.allocate_image = HwAcc::allocateImage;
    hwdec.init = HwAcc::init;
    hwdec.uninit = HwAcc::uninit;
    hwdec.probe = HwAcc::probe;
    hwdec.image_format = imgfmt;
    hwdec.init_decoder = HwAcc::initDecoder;
    return hwdec;
}

vd_lavc_hwdec mp_vd_lavc_vaapi = create_lavc_hwdec(HWDEC_VAAPI, IMGFMT_VAAPI);
vd_lavc_hwdec mp_vd_lavc_vdpau = create_lavc_hwdec(HWDEC_VDPAU, IMGFMT_VDPAU);
vd_lavc_hwdec mp_vd_lavc_vda   = create_lavc_hwdec(HWDEC_VDA,   IMGFMT_VDA);
