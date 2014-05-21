#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "stdafx.hpp"
#include "enum/deintmethod.hpp"
#include "enum/deintdevice.hpp"

class DeintOption {
public:
    DeintOption() = default;
    DeintOption(DeintMethod method, DeintDevice device, bool doubler)
        : method(method), device(device), doubler(doubler) { }
    auto operator == (const DeintOption &rhs) const -> bool
    {
        return method == rhs.method
               && doubler == rhs.doubler && device == rhs.device;
    }
    auto operator != (const DeintOption &rhs) const -> bool
        { return !operator == (rhs); }
    auto toString() const -> QString;
    static auto fromString(const QString &string) -> DeintOption;
// variables
    DeintMethod method = DeintMethod::None;
    DeintDevice device = DeintDevice::CPU;
    bool doubler = false;
};

#endif // DEINTINFO_HPP
