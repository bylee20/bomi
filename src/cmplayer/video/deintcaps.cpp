//#include "deintcaps.hpp"

//DeintCaps::DeintCaps()
//{
//}


//auto isAvailable() const -> bool { return m_method != DeintMethod::None && m_device != 0 && m_decoder != 0; }
//auto DeintCaps::list() -> QList<DeintCaps>
//{
//    static QList<DeintCaps> caps;
//    if (!caps.isEmpty())
//        return caps;
//    for (int i=0; i<DeintMethodInfo::size(); ++i) {
//        caps.push_back(DeintCaps());
//        caps.back().m_method = (DeintMethod)i;
//    }

//    auto set = [] (DeintMethod method, bool cpu, bool gl, bool doubler) {
//        auto &cap = caps[(int)method];
//        cap.m_method = method;
//        if (cpu) {
//            cap.m_device |= DeintDevice::CPU;
//            cap.m_decoder |= DecoderDevice::CPU;
//        }
//        if (gl) {
//            cap.m_device |= DeintDevice::OpenGL;
//            cap.m_decoder |= (DecoderDevice::CPU | DecoderDevice::GPU);
//        }
//        if (HwAcc::supports(method)) {
//            cap.m_device |= DeintDevice::GPU;
//            cap.m_decoder |= DecoderDevice::GPU;
//        }
//        cap.m_doubler = doubler;
//        return caps[(int)method];
//    };
//    //  method                       cpu    gl     doubler
//    set(DeintMethod::Bob           , false, true , true );
//    set(DeintMethod::LinearBob     , true , true , true );
//    set(DeintMethod::CubicBob      , true , false, true );
//    set(DeintMethod::LinearBlend   , true , false, false);
//    set(DeintMethod::Yadif         , true , false, true );
//    set(DeintMethod::Median        , true , false, true );
//    set(DeintMethod::MotionAdaptive, false, false, true );
//    return caps;
//}

//auto DeintCaps::default_(DecoderDevice dec) -> DeintCaps
//{
//    auto ref = list()[(int)DeintMethod::Bob];
//    Q_ASSERT(ref.m_decoder & dec);
//    DeintCaps caps;
//    caps.m_doubler = ref.m_doubler;
//    caps.m_decoder = (int)dec;
//    caps.m_device = (int)((dec == DecoderDevice::CPU) ? DeintDevice::CPU : DeintDevice::GPU);
//    return caps;
//}

//auto DeintCaps::toString() const -> QString
//{
//    QString text = DeintMethodInfo::name(m_method) % _L('|');
//    for (auto dec : DecoderDeviceInfo::items()) {
//        if (dec.value & m_decoder)
//            text += dec.name % _L(':');
//    }
//    text += "|";
//    for (auto dev : DeintDeviceInfo::items()) {
//        if (dev.value & m_device)
//            text += dev.name % _L(':');
//    }
//    text += "|" % _N(m_doubler);
//    return text;
//}

//auto DeintCaps::fromString(const QString &text) -> DeintCaps
//{
//    auto tokens = text.split('|', QString::SkipEmptyParts);
//    if (tokens.size() != 4)
//        return DeintCaps();
//    DeintCaps caps;
//    caps.m_method = DeintMethodInfo::from(tokens[0]);
//    for (auto dec : tokens[1].split(':', QString::SkipEmptyParts))
//        caps.m_decoder |= DecoderDeviceInfo::from(dec);
//    for (auto dev : tokens[2].split(':', QString::SkipEmptyParts))
//        caps.m_device |= DeintDeviceInfo::from(dev);
//    caps.m_doubler = tokens[3].toInt();
//    return caps;
//}
