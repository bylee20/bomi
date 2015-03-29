#include "win.hpp"

#ifdef Q_OS_WIN

#include "enum/deintmethod.hpp"
#include "enum/codecid.hpp"
#include <QScreen>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDesktopWidget>
#include <psapi.h>
#include <QWindow>
#include <QSettings>
#include <QFontDatabase>
#include <windowsx.h>

namespace OS {

auto defaultFixedFont() -> QFont
{
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    switch (QLocale(QLocale::system().name()).script()) {
    case QLocale::KoreanScript:
        font.setFamily(u"GulimChe"_q);
        break;
    case QLocale::JapaneseScript:
        font.setFamily(u"MS Gothic"_q);
        break;
    case QLocale::SimplifiedChineseScript:
        font.setFamily(u"NSimSun"_q);
        break;
    case QLocale::TraditionalChineseScript:
        font.setFamily(u"Ming Light"_q);
        break;
    default:
        break;
    }
    return font;
}

struct Win : public QObject {
    Win() { proc = GetCurrentProcess(); }
    EXECUTION_STATE executionState = 0;
    bool originalScreensaver = false;
    QHash<QWindow*, HIMC> imes;
    HANDLE shutdownToken = nullptr;
    HANDLE proc = nullptr;
    Dxva2Info dxva2;
};

static Win *d = nullptr;

auto initialize() -> void { _Renew(d); }
auto finalize() -> void { _Delete(d); }

WinWindowAdapter::WinWindowAdapter(QWindow* w)
    : WindowAdapter(w)
{
    qApp->installNativeEventFilter(this);
    w->installEventFilter(this);
    m_position = w->position();
}

auto WinWindowAdapter::isImeEnabled() const -> bool
{
    return !m_ime;
}

auto WinWindowAdapter::setImeEnabled(bool enabled) -> void
{
    if (enabled == isImeEnabled())
        return;
    if (enabled) {
        ImmAssociateContext((HWND)winId(), m_ime);
        m_ime = nullptr;
    } else
        m_ime = ImmAssociateContext((HWND)winId(), nullptr);
}

auto WinWindowAdapter::setFrameless(bool /*frameless*/) -> void
{
    return;
//    if (_Change(m_frameless, frameless) && !m_fs) {
//        auto w = window();
//        const auto g = w->geometry();
//        updateFrame();
//        w->resize(g.width() + 1, g.height());
//        w->resize(g.width(), g.height());
//    }
}

SIA setLayer(HWND hwnd, HWND layer) -> void
{
    SetWindowPos(hwnd, layer, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

auto WinWindowAdapter::setAlwaysOnTop(bool onTop) -> void
{
    m_onTop = onTop;
    setLayer((HWND)winId(), layer());
}

auto WinWindowAdapter::setFullScreen(bool fs) -> void
{
    if (!_Change(m_fs, fs))
        return;
    WindowAdapter::setFullScreen(fs);
    if (fs) {
        const auto hwnd = (HWND)winId();
        auto style = GetWindowLong(hwnd, GWL_STYLE);
        style |= WS_BORDER;
        SetWindowLong(hwnd, GWL_STYLE, style);
    }
}

auto WinWindowAdapter::eventFilter(QObject *obj, QEvent *ev) -> bool
{
    auto w = window();
    if (obj != w)
        return false;
    if (ev->type() != QEvent::Move)
        return false;
    m_position = static_cast<QMoveEvent*>(ev)->pos();
    return false;
}

auto WinWindowAdapter::nativeEventFilter(const QByteArray &, void *message, long *res) -> bool
{
    auto msg = static_cast<MSG*>(message);
    auto w = window();
    switch (msg->message) {
    case WM_MOVE: {
        if (!w->isVisible())
            return false;
        const auto x = GET_X_LPARAM(msg->lParam);
        const auto y = GET_Y_LPARAM(msg->lParam);
        QMoveEvent event({x, y}, m_position);
        qApp->sendEvent(w, &event);
        *res = 0;
        return true;
    } case WM_NCACTIVATE: {
        if (w->windowState() != Qt::WindowMaximized || msg->hwnd != (HWND)winId())
            return false;
        *res = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return true;
    } case WM_NCPAINT: {
        if (!w->isVisible() || msg->hwnd != (HWND)winId())
            return false;
        if (w->windowState() == Qt::WindowMaximized) {
            *res = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            return true;
        }
        if (!m_fs)
            return false;
        auto dc = GetWindowDC(msg->hwnd);
        if (!dc)
            return false;
        auto pen = CreatePen(PS_INSIDEFRAME, GetSystemMetrics(SM_CXFRAME), RGB(0, 0, 0));
        if (pen) {
            RECT rect;
            GetWindowRect(msg->hwnd, &rect);
            auto old = SelectObject(dc, pen);
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
            DeleteObject(SelectObject(dc, old));
        }
        ReleaseDC(msg->hwnd, dc);
        *res = 0;
        return true;
    } case WM_NCMOUSEMOVE: {
        if (!m_fs || msg->hwnd != (HWND)winId())
            return false;
        QPoint gpos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        QPoint pos = w->mapFromGlobal(gpos);
        QMouseEvent me(QEvent::MouseMove, pos, gpos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        qApp->sendEvent(w, &me);
        return false;
    } default:
        return false;
    }
}

auto createAdapter(QWindow *w) -> WindowAdapter*
{
    return new WinWindowAdapter(w);
}

auto setScreensaverEnabled(bool enabled) -> void
{
    const bool disabled = d->executionState & ES_DISPLAY_REQUIRED;
    if (disabled == !enabled)
        return;
    d->executionState = ES_CONTINUOUS;
    if (!enabled)
        d->executionState |= ES_DISPLAY_REQUIRED;
    SetThreadExecutionState(d->executionState);
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
    setSupportedCodecs({ CodecId::Mpeg2, CodecId::H264,
                         CodecId::Vc1, CodecId::Wmv3 });
    setSupportedDeints({ DeintMethod::Bob, DeintMethod::LinearBob,
                         DeintMethod::CubicBob, DeintMethod::Yadif });
}

auto getHwAcc() -> HwAcc*
{
    return &d->dxva2;
}

}

#endif
