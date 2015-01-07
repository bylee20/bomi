#include "hwacc.hpp"
#include "enum/deintmethod.hpp"
#include "misc/log.hpp"
#include <functional>
#include <va/va.h>
#if VA_CHECK_VERSION(0, 34, 0)
#include <va/va_compat.h>
#else
static constexpr VAProfile VAProfileNone = (VAProfile)-1;
#endif
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#ifdef None
#undef None
#endif

struct CodecInfo {
    QString name, desc;
    QVector<VAProfile> vaapiProfiles;
    QVector<VdpDecoderProfile> vdpauProfiles;
};

static const CodecInfo codecs[] = {
    { u"mpeg1video"_q, u"MPEG-1 video"_q, {}, {VDP_DECODER_PROFILE_MPEG1}},
    { u"mpeg2video"_q, u"MPEG-2 video"_q,
      {VAProfileMPEG2Simple, VAProfileMPEG2Main},
      {VDP_DECODER_PROFILE_MPEG2_SIMPLE, VDP_DECODER_PROFILE_MPEG2_MAIN} },
    { u"mpeg4"_q, u"MPEG-4 part 2"_q,
      {VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Main, VAProfileMPEG4Simple},
      {VDP_DECODER_PROFILE_MPEG4_PART2_ASP, VDP_DECODER_PROFILE_MPEG4_PART2_SP} },
    { u"wmv3"_q, u"Windows Media Video 9"_q,
      {VAProfileVC1Advanced, VAProfileVC1Main, VAProfileVC1Simple},
      {VDP_DECODER_PROFILE_VC1_ADVANCED, VDP_DECODER_PROFILE_VC1_MAIN, VDP_DECODER_PROFILE_VC1_SIMPLE} },
    { u"vc1"_q, u"SMPTE VC-1"_q, // same as wmv3
      {VAProfileVC1Advanced, VAProfileVC1Main, VAProfileVC1Simple},
      {VDP_DECODER_PROFILE_VC1_ADVANCED, VDP_DECODER_PROFILE_VC1_MAIN, VDP_DECODER_PROFILE_VC1_SIMPLE} },
    { u"h264"_q, u"H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"_q,
      {VAProfileH264Baseline, VAProfileH264ConstrainedBaseline, VAProfileH264High, VAProfileH264Main},
      {VDP_DECODER_PROFILE_H264_BASELINE, VDP_DECODER_PROFILE_H264_MAIN, VDP_DECODER_PROFILE_H264_HIGH} }
};

class HwAccInfo {
public:
    using GetErrorString = std::function<const char*(qint64)>;
    virtual ~HwAccInfo() { }
    auto getLogContext() const -> const char* { return m_log; }
    auto check(qint64 status, const QString &onError) -> bool
    {
        if (isOk(status))
            return true;
        _Error("Error: %%(0x%%) %%", m_error(status),
               QString::number(m_status, 16), onError);
        return false;
    }
    auto check(qint64 status, const char *onError = "") -> bool
        { return check(status, _L(onError)); }
    auto isOk(qint64 status) const -> bool { m_status = status; return isOk(); }
    auto isOk() const -> bool { return m_ok == m_status; }
    auto status() const -> qint64 { return m_status; }
    auto codecs() const -> QStringList { return m_codecs; }
    auto supports(const QString &c) const -> bool { return m_codecs.contains(c); }
    auto type() const { return m_type; }
    auto isNative() const { return m_native; }
    auto name() const { return m_name; }
    auto description() const { return m_desc; }
protected:
    auto setType(HwAcc::Type type) { m_type = type; }
    auto setLogContext(const char *ctx) { m_log = ctx; }
    auto setOkStatus(qint64 s) { m_ok = s; }
    auto setGetErrorStringFunction(GetErrorString &&func) { m_error = func; }
    auto setCodecList(const QStringList &c) { m_codecs = c; }
    auto setNative(bool native) { m_native = native; }
    auto setDescription(const QString &desc) { m_desc = desc; }
    auto setName(const QString &name) { m_name = name; }
    auto successStatus() const { return m_ok; }
private:
    const char *m_log = nullptr;
    qint64 m_ok = 0;
    std::function<const char*(qint64)> m_error = nullptr;
    mutable qint64 m_status = m_ok;
    QStringList m_codecs;
    HwAcc::Type m_type = HwAcc::None;
    bool m_native = false;
    QString m_name, m_desc;
};

class VaApiInfo : public HwAccInfo {
public:
    VaApiInfo()
    {
        setType(HwAcc::VaApiGLX);
        setLogContext("VAAPI");
        setName("vaapi"_a);
        setDescription(u"VA-API(Video Acceleration API)"_q);
        setOkStatus(VA_STATUS_SUCCESS);
        setGetErrorStringFunction([] (qint64 s) { return vaErrorStr(s); });
        const auto xdpy = QX11Info::display();
        auto display = vaGetDisplayGLX(xdpy);
        if (!check(display ? VA_STATUS_SUCCESS : VA_STATUS_ERROR_UNIMPLEMENTED,
                   "Cannot create VADisplay."))
            return;
        int major, minor;
        if (!check(vaInitialize(display, &major, &minor),
                   "Cannot initialize VA-API."))
            return;
        do {
            const auto vendor = QString::fromLatin1(vaQueryVendorString(display));
            if (vendor.contains("VDPAU"_a))
                break;
            setNative(true);
            auto size = vaMaxNumProfiles(display);
            QVector<VAProfile> profiles(size);
            if (!check(vaQueryConfigProfiles(display, profiles.data(), &size),
                       "No available profiles."))
                break;
            profiles.resize(size);
            QStringList codecs;
            for (auto profile : profiles) {
                int size = vaMaxNumEntrypoints(display);
                QVector<VAEntrypoint> entries(size);
                if (!isOk(vaQueryConfigEntrypoints(display, profile,
                                                        entries.data(), &size)))
                    continue;
                entries.resize(size);
                if (!entries.contains(VAEntrypointVLD))
                    continue;
                for (auto &codec : ::codecs) {
                    if (codec.vaapiProfiles.contains(profile)) {
                        if (!codecs.contains(codec.name))
                            codecs.push_back(codec.name);
                        break;
                    }
                }
            }
            isOk(VA_STATUS_SUCCESS);
            setCodecList(codecs);
        } while (false);
        vaTerminate(display);
    }
    auto type() const -> HwAcc::Type { return HwAcc::VaApiGLX; }
};

class VdpauInfo : public HwAccInfo {
    QVector<QByteArray> m_errors;
    VdpDevice m_device = 0;
    VdpGetProcAddress *m_proc = nullptr;
    template<class F>
    auto proc(VdpFuncId id, F &func) -> VdpStatus {
        if (m_proc && isOk())
            isOk(m_proc(m_device, id, &reinterpret_cast<void*&>(func)));
        return static_cast<VdpStatus>(status());
    }
public:
    VdpauInfo()
    {
        setType(HwAcc::VdpauX11);
        setLogContext("VDPAU");
        setName("vdpau"_a);
        setDescription(u"VDPAU(Video Decode and Presentation API for Unix)"_q);
        setOkStatus(VDP_STATUS_OK);
        setGetErrorStringFunction([this] (qint64 s) {
            if (m_errors.isEmpty() || s < 0 || s >= m_errors.size())
                return "Unknown error code";
            return m_errors[s].constData();
        });
        if (!check(vdp_device_create_x11(QX11Info::display(), QX11Info::appScreen(),
                                         &m_device, &m_proc), "Cannot intialize VDPAU device"))
            return;
        VdpGetErrorString *getErrorString = nullptr;
        VdpDeviceDestroy *deviceDestroy = nullptr;
        VdpDecoderQueryCapabilities *decoderQueryCaps = nullptr;
        VdpGetInformationString *getInformationString = nullptr;
#define PROC(id, f) proc(VDP_FUNC_ID_##id, f)
        PROC(GET_ERROR_STRING,                 getErrorString);
        PROC(DEVICE_DESTROY,                   deviceDestroy);
        PROC(DECODER_QUERY_CAPABILITIES,       decoderQueryCaps);
        PROC(GET_INFORMATION_STRING,           getInformationString);
#undef PROC
        if (getErrorString) {
            m_errors.resize(VDP_STATUS_ERROR);
            for (int i = 0; i < m_errors.size(); ++i)
                m_errors[i] = getErrorString(static_cast<VdpStatus>(i));
        }
        do {
            if (!check(status(), "Cannot get VDPAU functions."))
                break;
            char const *info = nullptr;
            if (!check(getInformationString(&info), "Cannot get VDPAU information."))
                break;
            if (QString::fromLatin1(info).contains("VAAPI"_a))
                break;
            setNative(true);
            auto supports = [=] (VdpDecoderProfile id) -> bool
            {
                VdpBool supported = VDP_FALSE;
                quint32 lv = 0, blocks = 0, w = 0, h = 0;
                return decoderQueryCaps(m_device, id, &supported, &lv, &blocks,
                                        &w, &h) == VDP_STATUS_OK && supported == VDP_TRUE;
            };
            QStringList codecs;
            for (auto &codec : ::codecs) {
                for (auto profile : codec.vdpauProfiles) {
                    if (supports(profile)) {
                        codecs.push_back(codec.name);
                        break;
                    }
                }
            }
            isOk(VDP_STATUS_OK);
            setCodecList(codecs);
        } while (false);
        if (deviceDestroy)
            deviceDestroy(m_device);
        m_device = 0;
    }
    auto type() const -> HwAcc::Type { return HwAcc::VdpauX11; }
};

static HwAccInfo *api = nullptr;

DECLARE_LOG_CONTEXT(HwAcc)

auto HwAcc::initialize() -> void
{
#ifdef Q_OS_LINUX
    api = new VaApiInfo;
    if (!api->isNative()) {
        delete api;
        api = new VdpauInfo;
        if (!api->isNative())
            _Delete(api);
    }
#endif
    if (api && !api->isOk()) {
        _Info("Failed to initalize hardware acceration API.");
        _Delete(api);
    }
    if (api)
        _Info("Initialized hardware acceleration API: %%.", api->name());
    else
        _Info("No available hardware acceleration API.");

}

auto HwAcc::finalize() -> void
{
    _Delete(api);
}

auto HwAcc::isAvailable() -> bool
{
    return api && api->isOk();
}

auto HwAcc::type() -> Type
{
    return api ? api->type() : None;
}

auto HwAcc::name() -> QString
{
    return api ? api->name() : QString();
}

auto HwAcc::description() -> QString
{
    return api ? api->description() : QString();
}

auto HwAcc::supports(const QString &codec) -> bool
{
    return api && api->supports(codec);
}

auto HwAcc::supports(DeintMethod method) -> bool
{
#ifdef Q_OS_LINUX
//#ifdef USE_VAVPP
//    auto filter = VaApi::filter(VAProcFilterDeinterlacing);
//    return filter && filter->supports(VaApi::toVAType(method));
//#else
    return method == DeintMethod::Bob;
#endif
//#endif
    return false;
}

auto HwAcc::fullDeintList() -> QList<DeintMethod>
{
    static auto list = QList<DeintMethod>() << DeintMethod::Bob;
    return list;
}

auto HwAcc::fullCodecList() -> QStringList
{
    static const QStringList codecs = [] () {
        QStringList list;
        for (auto &codec : ::codecs)
            list.push_back(codec.name);
        return list;
    }();
    return codecs;
}

auto HwAcc::codecDescription(const QString &codec) -> QString
{
    for (auto &c : codecs) {
        if (c.name == codec)
            return c.desc;
    }
    return QString();
}
