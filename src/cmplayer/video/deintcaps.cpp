#include "deintcaps.hpp"
#include "hwacc.hpp"
#include "misc/json.hpp"

template<>
struct JsonIO<DeintCaps> {
    static auto io()
    {
        static const auto io = _JIO<DeintCaps>(\
            _JE(u"method"_q, &DeintCaps::m_method),\
            _JE(u"decoders"_q, &DeintCaps::m_decoders),\
            _JE(u"devices"_q, &DeintCaps::m_devices),\
            _JE(u"doubler"_q, &DeintCaps::m_doubler)\
        );
        return &io;
    }
};

static const auto jio = JsonIO<DeintCaps>::io();

auto DeintCaps::toJson() const -> QJsonObject
{
    return jio->toJson(*this);
}

auto DeintCaps::setFromJson(const QJsonObject &json) -> bool
{
    return jio->fromJson(*this, json);
}

//decltype(DeintCaps::m_method) a;



//auto DeintCaps::jio() -> const void*
//{
//    static const auto io = JSON_IO_DEINT_CAPS;
//    return &io;
//}

//auto json_io(const DeintCaps*) -> const decltype(JSON_IO_DEINT_CAPS)*
//{

//}


auto DeintCaps::list() -> QList<DeintCaps>
{
    static QList<DeintCaps> caps;
    if (!caps.isEmpty())
        return caps;
    for (int i=0; i<DeintMethodInfo::size(); ++i) {
        caps.push_back(DeintCaps());
        caps.back().m_method = (DeintMethod)i;
    }

    auto set = [] (DeintMethod method, bool cpu, bool gl, bool doubler) {
        auto &cap = caps[(int)method];
        cap.m_method = method;
        if (cpu) {
            cap.m_devices |= DeintDevice::CPU;
            cap.m_decoders |= DecoderDevice::CPU;
        }
        if (gl) {
            cap.m_devices |= DeintDevice::OpenGL;
            cap.m_decoders |= (DecoderDevice::CPU | DecoderDevice::GPU);
        }
//        if (HwAcc::supports(method)) {
//            cap.m_devices |= DeintDevice::GPU;
//            cap.m_decoders |= DecoderDevice::GPU;
//        }
        cap.m_doubler = doubler;
        return caps[(int)method];
    };
    //  method                       cpu    gl     doubler
    set(DeintMethod::Bob           , false, true , true );
    set(DeintMethod::LinearBob     , true , true , true );
    set(DeintMethod::CubicBob      , true , false, true );
    set(DeintMethod::LinearBlend   , true , false, false);
    set(DeintMethod::Yadif         , true , false, true );
    set(DeintMethod::Median        , true , false, true );
    set(DeintMethod::MotionAdaptive, false, false, true );
    return caps;
}

auto DeintCaps::default_(DecoderDevice dec) -> DeintCaps
{
    auto ref = list()[(int)DeintMethod::Bob];
    Q_ASSERT(ref.m_decoders.contains(dec));
    DeintCaps caps;
    caps.m_doubler = ref.m_doubler;
    caps.m_decoders = dec;
    caps.m_devices = dec == DecoderDevice::CPU ? DeintDevice::CPU
                                               : DeintDevice::GPU;
    return caps;
}

auto DeintCaps::toString() const -> QString
{
    QString text = DeintMethodInfo::name(m_method) % '|'_q;
    for (auto dec : DecoderDeviceInfo::items()) {
        if (m_decoders.contains(dec.value))
            text += dec.name % ':'_q;
    }
    text += '|'_q;
    for (auto dev : DeintDeviceInfo::items()) {
        if (m_devices.contains(dev.value))
            text += dev.name % ':'_q;
    }
    text += '|'_q % _N(m_doubler);
    return text;
}

auto DeintCaps::fromString(const QString &text) -> DeintCaps
{
    auto tokens = text.split('|'_q, QString::SkipEmptyParts);
    if (tokens.size() != 4)
        return DeintCaps();
    DeintCaps caps;
    caps.m_method = DeintMethodInfo::from(tokens[0]);
    for (auto dec : tokens[1].split(':'_q, QString::SkipEmptyParts))
        caps.m_decoders |= DecoderDeviceInfo::from(dec);
    for (auto dev : tokens[2].split(':'_q, QString::SkipEmptyParts))
        caps.m_devices |= DeintDeviceInfo::from(dev);
    caps.m_doubler = tokens[3].toInt();
    return caps;
}
