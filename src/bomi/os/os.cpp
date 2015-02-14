#include "os/os.hpp"
#include "enum/codecid.hpp"
#include "enum/deintmethod.hpp"

namespace OS {

auto getHwAcc() -> HwAcc*;

auto hwAcc() -> HwAcc*
{
    auto api = getHwAcc();
    if (!api) {
        static HwAcc none;
        api = &none;
    }
    return api;
}

auto HwAcc::fullCodecList() -> QList<CodecId>
{
    static const QList<CodecId> ids = QList<CodecId>()
        << CodecId::Mpeg1 << CodecId::Mpeg2 << CodecId::Mpeg4
        << CodecId::H264 << CodecId::Vc1 << CodecId::Wmv3;
    return ids;
}

struct ApiInfo {
    HwAcc::Api api;
    QString name, desc;
};

const std::array<ApiInfo, HwAcc::NoApi> s_infos = [] () {
    std::array<ApiInfo, HwAcc::NoApi> ret;
#define SET(a, n, d) {ret[HwAcc::a] = {HwAcc::a, n, d};}
    SET(VaApiGLX, u"vaapi"_q, u"VA-API(Video Acceleration API)"_q);
    SET(VdpauX11, u"vdpau"_q, u"VDPAU(Video Decode and Presentation API for Unix)"_q );
    SET(Dxva2Copy, u"dxva2-copy"_q, u"DXVA 2.0(DirectX Video Accelaction 2.0)"_q );
#undef SET
    return ret;
}();

struct HwAcc::Data {
    Api api = NoApi;
    QList<CodecId> codecs;
    QList<DeintMethod> deints;
};

HwAcc::HwAcc(Api api)
    : d(new Data)
{
    d->api = api;
}

HwAcc::~HwAcc()
{
    delete d;
}

auto HwAcc::isAvailable() const -> bool
{
    return d->api != NoApi;
}

auto HwAcc::supports(CodecId codec) -> bool
{
    return d->codecs.contains(codec);
}

auto HwAcc::supports(DeintMethod method) -> bool
{
    return d->deints.contains(method);
}

auto HwAcc::api() const -> Api
{
    return d->api;
}

auto HwAcc::name() const -> QString
{
    return name(d->api);
}

auto HwAcc::description() const -> QString
{
    return description(d->api);
}

auto HwAcc::name(Api api) -> QString
{
    if (_InRange0(api, NoApi))
        return s_infos[api].name;
    return QString();
}

auto HwAcc::description(Api api) -> QString
{
    if (_InRange0(api, NoApi))
        return s_infos[api].desc;
    return QString();
}

auto HwAcc::api(const QString &name) -> Api
{
    for (auto &info : s_infos) {
        if (info.name == name)
            return info.api;
    }
    return NoApi;
}

auto HwAcc::setSupportedCodecs(const QList<CodecId> &codecs) -> void
{
    d->codecs = codecs;
}

auto HwAcc::setSupportedDeints(const QList<DeintMethod> &deints) -> void
{
    d->deints = deints;
}

auto HwAcc::download(mp_hwdec_ctx *, const mp_image *, mp_image_pool *) -> mp_image*
{
    return nullptr;
}

}
