#include "win.hpp"

#ifdef Q_OS_WIN

#include "enum/deintmethod.hpp"
#include "enum/codecid.hpp"
#include "misc/log.hpp"
#include "player/app.hpp"
#include <QScreen>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDesktopWidget>
#include <psapi.h>
#include <QWindow>
#include <QSettings>
#include <QFontDatabase>
#include <windowsx.h>
#include <Shlobj.h>

DECLARE_LOG_CONTEXT(OS)

namespace OS {

SIA isAdmin() -> bool
{
    SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
    PSID sid = nullptr;
    if (!AllocateAndInitializeSid(&auth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid))
        return false;
    BOOL check = FALSE;
    const auto res = CheckTokenMembership(nullptr, sid, &check);
    FreeSid(sid);
    return (check && res);
}

SIA uac(QWindow *w, const QString &param) -> bool
{
    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(nullptr, path, ARRAYSIZE(path)))
        return false;
    QVector<wchar_t> paramBuf(param.size() * 2);
    param.toWCharArray(paramBuf.data());
    SHELLEXECUTEINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.lpVerb = L"runas";
    info.lpFile = path;
    info.lpParameters = paramBuf.data();
    info.nShow = SW_NORMAL;
    if (w)
        info.hwnd = (HWND)w->winId();
    if (ShellExecuteEx(&info))
        return true;
    return false;
}

auto canAssociateFileTypes() -> bool { return true; }

SIA openRegistry(const QString &group) -> QSharedPointer<QSettings>
{
    return QSharedPointer<QSettings>(new QSettings(group, QSettings::NativeFormat));
}

auto unassociateFileTypes(QWindow *w, bool global) -> bool
{
    if (global && !isAdmin())
        return uac(w, u"--win-unassoc"_q);

    auto s = openRegistry(global ? u"HKEY_LOCAL_MACHINE\\Software"_q
                                 : u"HKEY_CURRENT_USER\\Software"_q);
    s->remove(u"bomi"_q);
    s->remove(u"Microsoft/Windows/CurrentVersion/App Paths/bomi.exe"_q);
    s->remove(u"RegisteredApplications/bomi"_q);

    s->beginGroup(u"Classes"_q);
    s->remove(u"Applications/bomi.exe"_q);
    const auto children = s->childGroups();
    for (auto &child : children) {
        if (child.startsWith('.'_q))
            s->remove(child % u"/OpenWithProgids/bomi"_q % child);
        else if (child.startsWith("bomi."_a))
            s->remove(child);
    }
    s->endGroup();

    s.clear();
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, nullptr, nullptr);
    return true;
}

auto associateFileTypes(QWindow *w, bool global, const QStringList &exts) -> bool
{
    if (global && !isAdmin())
        return uac(w, u"--win-assoc "_q % exts.join(','_q));

    auto s = openRegistry(global ? u"HKEY_LOCAL_MACHINE\\Software"_q
                                 : u"HKEY_CURRENT_USER\\Software"_q);

    const auto exec = QDir::toNativeSeparators(qApp->applicationFilePath());
    s->beginGroup(u"Microsoft/Windows/CurrentVersion/App Paths/bomi.exe"_q);
    s->setValue(u"."_q, exec);
    s->setValue(u"Path"_q, QDir::toNativeSeparators(qApp->applicationDirPath()));
    s->endGroup();

    s->beginGroup(u"bomi/Capabilities"_q);
    s->setValue(u"ApplicationDescription"_q, u"multimedia player"_q);
    s->setValue(u"ApplicationName"_q, cApp.displayName());
    s->beginGroup(u"FileAssociations"_q);
    for (auto &ext : exts)
        s->setValue('.'_q % ext, QString("bomi."_a % ext));
    s->endGroup();
    s->endGroup();

    s->setValue(u"RegisteredApplications/bomi"_q, uR"(Software\bomi\Capabilities)"_q);

    const QString cmd = '"'_q % exec % "\" \"%1\""_a;
    auto shell = [&] () {
        s->setValue(u"shell/play/command/."_q, cmd);
        s->setValue(u"shell/play/FriendlyAppName"_q, cApp.displayName());
        s->setValue(u"shell/open/command/."_q, cmd);
        s->setValue(u"shell/open/FriendlyAppName"_q, cApp.displayName());
    };

    s->beginGroup(u"Classes"_q);

    s->beginGroup(u"Applications/bomi.exe"_q);
    shell();
    s->endGroup();

    for (auto &ext : exts) {
        const QString id = "bomi."_a % ext;
        s->beginGroup(id);
        s->setValue(u"."_q, _DescriptionForSuffix(ext));
        shell();
        s->endGroup();

        s->beginGroup('.'_q % ext);
        s->setValue(u"."_q, id);
        s->setValue("OpenWithProgids/"_a % id, QString());
        s->endGroup();
    }
    s->endGroup();

    s.clear();
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, nullptr, nullptr);
    return false;
}

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
