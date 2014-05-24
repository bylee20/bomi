#ifndef DEINTCAPS_HPP
#define DEINTCAPS_HPP

#include "stdafx.hpp"
#include "enum/deintmethod.hpp"
#include "enum/deintdevice.hpp"
#include "enum/decoderdevice.hpp"

class DeintCaps {
public:
    auto method() const -> DeintMethod { return m_method; }
    auto hwdec() const -> bool { return supports(DecoderDevice::GPU); }
    auto swdec() const -> bool { return supports(DecoderDevice::CPU); }
    auto doubler() const -> bool { return m_doubler; }
    auto supports(DeintDevice dev) const -> bool { return m_devices.contains(dev); }
    auto supports(DecoderDevice dev) const -> bool { return m_decoders.contains(dev); }
    auto isAvailable() const -> bool;
    auto toString() const -> QString;
    static auto fromString(const QString &text) -> DeintCaps;
    static auto default_(DecoderDevice dec) -> DeintCaps;
    static auto list() -> QList<DeintCaps>;
private:
    friend class DeintWidget;
    DeintMethod m_method = DeintMethod::None;
    DecoderDevices m_decoders = 0;
    DeintDevices m_devices = 0;
    bool m_doubler = false;
};

inline auto DeintCaps::isAvailable() const -> bool
{
    return m_method != DeintMethod::None && m_devices != 0 && m_decoders != 0;
}

#endif // DEINTCAPS_HPP
