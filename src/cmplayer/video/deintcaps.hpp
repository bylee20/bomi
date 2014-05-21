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
    auto supports(DeintDevice dev) const -> bool { return m_device & dev; }
    auto supports(DecoderDevice dev) const -> bool { return m_decoder & dev; }
    auto isAvailable() const -> bool;
    auto toString() const -> QString;
    static auto fromString(const QString &text) -> DeintCaps;
    static auto default_(DecoderDevice dec) -> DeintCaps;
    static auto list() -> QList<DeintCaps>;
private:
    friend class DeintWidget;
    DeintMethod m_method = DeintMethod::None;
    int m_decoder = 0, m_device = 0;
    bool m_doubler = false;
};

inline auto DeintCaps::isAvailable() const -> bool
{
    return m_method != DeintMethod::None && m_device != 0 && m_decoder != 0;
}

#endif // DEINTCAPS_HPP
