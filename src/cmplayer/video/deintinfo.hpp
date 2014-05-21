#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "stdafx.hpp"
#include "enum/deintmethod.hpp"
#include "enum/deintdevice.hpp"
#include "enum/decoderdevice.hpp"

class DeintOption {
public:
    DeintOption() {}
    DeintOption(DeintMethod method, DeintDevice device, bool doubler)
    : method(method), device(device), doubler(doubler) {}
    bool operator == (const DeintOption &rhs) const {
        return method == rhs.method && doubler == rhs.doubler && device == rhs.device;
    }
    bool operator != (const DeintOption &rhs) const { return !operator == (rhs); }
    auto toString() const -> QString;
    static auto fromString(const QString &string) -> DeintOption;
// variables
    DeintMethod method = DeintMethod::None;
    DeintDevice device = DeintDevice::CPU;
    bool doubler = false;
};

class DeintCaps {
public:
    static QList<DeintCaps> list();
    auto method() const -> DeintMethod { return m_method; }
    auto hwdec() const -> bool { return supports(DecoderDevice::GPU); }
    auto swdec() const -> bool { return supports(DecoderDevice::CPU); }
    auto doubler() const -> bool { return m_doubler; }
    auto supports(DeintDevice dev) const -> bool { return m_device & dev; }
    auto supports(DecoderDevice dev) const -> bool { return m_decoder & dev; }
    auto isAvailable() const -> bool { return m_method != DeintMethod::None && m_device != 0 && m_decoder != 0; }
    auto toString() const -> QString;
    static auto fromString(const QString &text) -> DeintCaps;
    static auto default_(DecoderDevice dec) -> DeintCaps;
private:
    friend class DeintWidget;
    DeintMethod m_method = DeintMethod::None;
    int m_decoder = 0, m_device = 0;
    bool m_doubler = false;
};



class DeintWidget : public QWidget {
    Q_OBJECT
public:
    DeintWidget(DecoderDevice decoder, QWidget *parent = nullptr);
    ~DeintWidget();
    auto set(const DeintCaps &caps) -> void;
    auto get() const -> DeintCaps;
    static auto informations() -> QString;
private:
    static constexpr auto GPU = DeintDevice::GPU;
    static constexpr auto CPU = DeintDevice::CPU;
    static constexpr auto OpenGL = DeintDevice::OpenGL;
    struct Data;
    Data *d;
};

#endif // DEINTINFO_HPP
