#include "win.hpp"

#ifdef Q_OS_WIN

#include "enum/deintmethod.hpp"
#include "enum/codecid.hpp"
#include <qt_windows.h>
#include <QScreen>
#include <QDesktopWidget>
#include <psapi.h>
#include <QWindow>

namespace OS {

static const auto REGISTRY_DESKTOP_PATH = u"HKEY_CURRENT_USER\\Control Panel\\Desktop"_q;
static const auto REGISTRY_SCREENSAVER_KEY = u"SCRNSAVE.EXE"_q;

struct Win : public QObject {
    Win() {
        originalScreensaver = isScreensaverEnabled();
        proc = GetCurrentProcess();
    }
    auto isScreensaverEnabled() -> bool
    {
        QSettings s(REGISTRY_DESKTOP_PATH, QSettings::NativeFormat);
        return !s.value(REGISTRY_SCREENSAVER_KEY).isNull();
    }

    bool originalScreensaver = false;

    struct Window {
        bool onTop = false;
        bool fullScreen = false;
        QRect prevGeometry;
        DWORD prevStyle = 0;
        auto layer() const -> HWND
        { return onTop ? HWND_TOPMOST : HWND_NOTOPMOST; }
    };

    QHash<QWidget*, Window> windows;
    QHash<QWindow*, HIMC> imes;
    HANDLE shutdownToken = nullptr;
    HANDLE proc = nullptr;
    Dxva2Info dxva2;
};

static Win *d = nullptr;

auto initialize() -> void { _Renew(d); }
auto finalize() -> void { _Delete(d); }

auto setImeEnabled(QWindow *w, bool enabled) -> void
{
    auto &ime = d->imes[w];
    if (enabled) {
        if (ime) {
            ImmAssociateContext((HWND)w->winId(), ime);
            ime = nullptr;
        }
    } else
        ime = ImmAssociateContext((HWND)w->winId(), nullptr);
}

auto setAlwaysOnTop(QWidget *w, bool onTop) -> void
{
    auto &win = d->windows[w];
    win.onTop = onTop;
    SetWindowPos((HWND)w->winId(), win.layer(), 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

auto isFullScreen(const QWidget *w) -> bool
{
    return d->windows.value(const_cast<QWidget*>(w)).fullScreen;
}

auto isAlwaysOnTop(const QWidget *w) -> bool
{
    return d->windows.value(const_cast<QWidget*>(w)).onTop;
}

auto setFullScreen(QWidget *w, bool fs) -> void
{
    auto &win = d->windows[w];
    if (win.fullScreen == fs)
        return;
    win.fullScreen = fs;
    auto g = win.prevGeometry;
    auto style = win.prevStyle;
    auto layer = win.layer();
    const auto hwnd = (HWND)w->winId();
    if (fs) {
        win.prevGeometry = w->frameGeometry();
        win.prevStyle = GetWindowLong(hwnd, GWL_STYLE);
        g = qApp->desktop()->screenGeometry(w);
        g.adjust(-1, -1, 1, 1);
        style = win.prevStyle & ~(WS_DLGFRAME | WS_THICKFRAME);
        layer = HWND_NOTOPMOST;
    }
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, layer, g.x(), g.y(), g.width(), g.height(),
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

auto setScreensaverDisabled(bool disabled) -> void
{
    const auto active = disabled ? false : d->originalScreensaver;
    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, active, 0, SPIF_SENDWININICHANGE);
}

auto processTime() -> quint64
{
    ULARGE_INTEGER creation, exit, kernel, user;
    if (!GetProcessTimes(d->proc, (LPFILETIME)&creation.u, (LPFILETIME)&exit.u,
                         (LPFILETIME)&kernel.u, (LPFILETIME)&user.u))
        return 0;
    return (kernel.QuadPart + user.QuadPart)/10;
}

auto systemTime() -> quint64
{
    ULARGE_INTEGER idle, kernel, user;
    if (!GetSystemTimes((LPFILETIME)&idle, (LPFILETIME)&kernel,
                        (LPFILETIME)&user))
        return 0;
    return (idle.QuadPart + kernel.QuadPart + user.QuadPart)/10;
}

auto totalMemory() -> double
{
    MEMORYSTATUSEX memory;
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatusEx(&memory);
    return memory.ullTotalPhys/double(1024*1024);
}

auto usingMemory() -> double
{
    PROCESS_MEMORY_COUNTERS_EX counters;
    if (!GetProcessMemoryInfo(d->proc, (PPROCESS_MEMORY_COUNTERS)&counters,
                              sizeof(counters)))
        return 0.0;
    return counters.WorkingSetSize/double(1024*1024);
}

auto canShutdown() -> bool
{
    if (d->shutdownToken)
        return true;
    HANDLE token = nullptr;
    if(!OpenProcessToken(d->proc, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;

    TOKEN_PRIVILEGES p;
    LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &p.Privileges[0].Luid);
    p.PrivilegeCount = 1;
    p.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(token, false, &p, 0,
                          static_cast<PTOKEN_PRIVILEGES>(nullptr), 0);
    if(GetLastError() != ERROR_SUCCESS)
        return false;

    d->shutdownToken = token;
    return true;
}

auto shutdown() -> bool
{
    if (!d->shutdownToken)
        canShutdown();
    if (!d->shutdownToken)
        return false;
    return ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,
                         SHTDN_REASON_MAJOR_OPERATINGSYSTEM
                         | SHTDN_REASON_MINOR_UPGRADE
                         | SHTDN_REASON_FLAG_PLANNED);
}

auto opticalDrives() -> QStringList
{
    return QStringList();
}

auto refreshRate() -> qreal
{
    return qApp->primaryScreen()->refreshRate();
}

Dxva2Info::Dxva2Info()
    : HwAcc(Dxva2Copy)
{
    setSupportedCodecs(QList<CodecId>() << CodecId::Mpeg2 << CodecId::H264
                       << CodecId::Vc1 << CodecId::Wmv3);
    setSupportedDeints(QList<DeintMethod>() << DeintMethod::Bob
                       << DeintMethod::LinearBob << DeintMethod::CubicBob
                       << DeintMethod::LinearBlend << DeintMethod::Yadif
                       << DeintMethod::Median);
}

auto getHwAcc() -> HwAcc*
{
    return &d->dxva2;
}

}

#endif
