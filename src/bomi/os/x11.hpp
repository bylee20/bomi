#ifndef X11_HPP
#define X11_HPP

#include "os/os.hpp"

#ifdef Q_OS_LINUX

#include "misc/log.hpp"
#include "enum/deintmethod.hpp"
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

namespace OS {

auto createAdapter(QWidget *w) -> WindowAdapter*;

class X11WindowAdapter : public WindowAdapter {
public:
    X11WindowAdapter(QWidget* w): WindowAdapter(w) { }
    auto setFullScreen(bool fs) -> void final;
    auto isAlwaysOnTop() const -> bool final;
    auto setAlwaysOnTop(bool onTop) -> void final;
};

class HwAccX11 : public HwAcc {
public:
    using GetErrorString = std::function<const char*(qint64)>;
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
    auto isNative() const { return m_native; }
protected:
    HwAccX11(Api api): HwAcc(api)
        { setSupportedDeints(QList<DeintMethod>() << DeintMethod::Bob);}
    auto setLogContext(const char *ctx) { m_log = ctx; }
    auto setOkStatus(qint64 s) { m_ok = s; }
    auto setGetErrorStringFunction(GetErrorString &&func) { m_error = func; }
    auto setNative(bool native) { m_native = native; }
    auto successStatus() const { return m_ok; }
private:
    const char *m_log = nullptr;
    qint64 m_ok = 0;
    std::function<const char*(qint64)> m_error = nullptr;
    mutable qint64 m_status = m_ok;
    bool m_native = false;
};

struct VaApiInfo : public HwAccX11 {
    VaApiInfo();
    auto download(mp_hwdec_ctx *ctx, const mp_image *mpi,
                  mp_image_pool *pool) -> mp_image* final;
};

struct VdpauInfo : public HwAccX11 {
    VdpauInfo();
    auto download(mp_hwdec_ctx *ctx, const mp_image *mpi,
                  mp_image_pool *pool) -> mp_image* final;
private:
    QVector<QByteArray> m_errors;
    VdpDevice m_device = 0;
    VdpGetProcAddress *m_proc = nullptr;
    template<class F>
    auto proc(VdpFuncId id, F &func) -> VdpStatus {
        if (m_proc && isOk())
            isOk(m_proc(m_device, id, &reinterpret_cast<void*&>(func)));
        return static_cast<VdpStatus>(status());
    }
};

}

#endif

#endif // X11_HPP
