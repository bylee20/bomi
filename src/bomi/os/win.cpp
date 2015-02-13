#include "win.hpp"

#ifdef Q_OS_WIN

#include <qt_windows.h>
#include <QScreen>

namespace OS {

static const auto REGISTRY_DESKTOP_PATH = u"HKEY_CURRENT_USER\\Control Panel\\Desktop"_q;
static const auto REGISTRY_SCREENSAVER_KEY = u"SCRNSAVE.EXE"_q;

struct Win : public QObject {
    Win() {
        originalScreensaver = isScreensaverEnabled();
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
    HANDLE shutdownToken = nullptr;
};

static Win *d = nullptr;

auto initialize() -> void { _Renew(d); }
auto finalize() -> void { _Delete(d); }

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
    if (!GetProcessTimes(GetCurrentProcess(),
                         (LPFILETIME)&creation.u, (LPFILETIME)&exit.u,
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

auto canShutdown() -> bool
{
    if (d->shutdownToken)
        return true;
    HANDLE token = nullptr;
    const auto proc = GetCurrentProcess();
    if(!OpenProcessToken(proc, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
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

}

#endif
