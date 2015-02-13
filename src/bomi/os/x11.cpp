#include "os.hpp"

#ifdef Q_OS_LINUX

#include "misc/log.hpp"
#include "tmp/algorithm.hpp"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtX11Extras/QX11Info>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

extern "C" {
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/screensaver.h>
}

namespace OS {

DECLARE_LOG_CONTEXT(X11)

enum XcbAtom {
    _NET_WM_STATE,
    _NET_WM_STATE_MODAL,
    _NET_WM_STATE_STICKY,
    _NET_WM_STATE_MAXIMIZED_VERT,
    _NET_WM_STATE_MAXIMIZED_HORZ,
    _NET_WM_STATE_SHADED,
    _NET_WM_STATE_SKIP_TASKBAR,
    _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_HIDDEN,
    _NET_WM_STATE_FULLSCREEN,
    _NET_WM_STATE_ABOVE,
    _NET_WM_STATE_BELOW,
    _NET_WM_STATE_DEMANDS_ATTENTION,
    _NET_WM_STATE_STAYS_ON_TOP,
    XcbAtomEnd,
    XcbAtomBegin = _NET_WM_STATE
};

static auto getAtom(xcb_connection_t *conn, const char *name) -> xcb_atom_t
{
    auto cookie = xcb_intern_atom(conn, 0, strlen(name), name);
    auto reply = xcb_intern_atom_reply(conn, cookie, nullptr);
    if (!reply)
        return 0;
    auto ret = reply->atom;
    free(reply);
    return ret;
}

struct X11 : public QObject {
    X11();
    ~X11();

    struct {
        QTimer reset;
        QDBusInterface *iface = nullptr;
        QDBusReply<uint> reply;
        bool inhibit = false, xss = false, gnome = false;
    } ss;

    xcb_connection_t *connection = nullptr;
    xcb_window_t root = 0;
    Display *display = nullptr;
    xcb_atom_t atoms[XcbAtomEnd];

    auto sendState(QWidget *widget, bool on,
                   XcbAtom a1, XcbAtom a2 = XcbAtomEnd) -> void
    {
        if (!widget)
            return;
        const auto window = widget->winId();
        xcb_client_message_event_t event;
        memset(&event, 0, sizeof(event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.window = window;
        event.type = atoms[_NET_WM_STATE];
        event.data.data32[0] = on ? 1 : 0;
        event.data.data32[1] = atoms[a1];
        if (a2 != XcbAtomEnd)
            event.data.data32[2] = atoms[a2];
        const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_send_event(connection, 0, root, mask, (const char *)&event);
        xcb_flush(connection);
    }
};

static X11 *d = nullptr;

auto initialize() -> void { _Renew(d); }
auto finalize() -> void { _Delete(d); }


X11::X11()
{
    connection = QX11Info::connection();
    display = QX11Info::display();
    root = QX11Info::appRootWindow();

    Q_ASSERT(connection);

#define GET_ATOM(id) {atoms[id] = getAtom(connection, #id);}
    GET_ATOM(_NET_WM_STATE);
    GET_ATOM(_NET_WM_STATE_MODAL);
    GET_ATOM(_NET_WM_STATE_STICKY);
    GET_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
    GET_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
    GET_ATOM(_NET_WM_STATE_SHADED);
    GET_ATOM(_NET_WM_STATE_SKIP_TASKBAR);
    GET_ATOM(_NET_WM_STATE_SKIP_PAGER);
    GET_ATOM(_NET_WM_STATE_HIDDEN);
    GET_ATOM(_NET_WM_STATE_FULLSCREEN);
    GET_ATOM(_NET_WM_STATE_ABOVE);
    GET_ATOM(_NET_WM_STATE_BELOW);
    GET_ATOM(_NET_WM_STATE_DEMANDS_ATTENTION);
    GET_ATOM(_NET_WM_STATE_STAYS_ON_TOP);
#undef GET_ATOM

    ss.reset.setInterval(20000);
    connect(&ss.reset, &QTimer::timeout, this, [=] () {
        Q_ASSERT(ss.xss);
        _Debug("Force XCB_SCREEN_SAVER_RESET.");
        xcb_force_screen_saver(connection, XCB_SCREEN_SAVER_RESET);
        xcb_flush(connection);
    });
}

X11::~X11()
{
    delete ss.iface;
}

template<class T>
static inline QSharedPointer<T> _Reply(T *t) { return QSharedPointer<T>(t, free); }

auto refreshRate() -> qreal
{
    auto sr = _Reply(xcb_randr_get_screen_resources_current_reply(
        d->connection,
        xcb_randr_get_screen_resources_current_unchecked(d->connection, d->root),
        nullptr));
    const auto len = xcb_randr_get_screen_resources_current_crtcs_length(sr.data());
    const int n = qApp->desktop()->screenNumber(qApp->topLevelWidgets().value(0));
    if (len < 1 || n >= len)
        return -1;
    auto crtc = xcb_randr_get_screen_resources_current_crtcs(sr.data())[n];
    auto ci = _Reply(xcb_randr_get_crtc_info_reply(
        d->connection,
        xcb_randr_get_crtc_info_unchecked(d->connection, crtc, XCB_CURRENT_TIME),
        nullptr));

    auto mode_refresh = [] (xcb_randr_mode_info_t *mode_info) -> double
    {
        double vtotal = mode_info->vtotal;
        if (mode_info->mode_flags & XCB_RANDR_MODE_FLAG_DOUBLE_SCAN)
            vtotal *= 2;
        if (mode_info->mode_flags & XCB_RANDR_MODE_FLAG_INTERLACE)
            vtotal /= 2;
        if (mode_info->htotal && vtotal)
            return ((double) mode_info->dot_clock / ((double) mode_info->htotal * (double) vtotal));
        return -1;
    };

    auto it = xcb_randr_get_screen_resources_current_modes_iterator(sr.data());
    while (it.rem) {
        if (it.data->id == ci->mode)
            return mode_refresh(it.data);
        xcb_randr_mode_info_next(&it);
    }
    return -1;
}

auto setScreensaverDisabled(bool disabled) -> void
{
    auto &s = d->ss;
    if (s.inhibit == disabled)
        return;
    if (!s.iface && !s.xss) {
        _Debug("Initialize screensaver functions.");
        _Debug("Trying to connect 'org.gnome.SessionManager'.");
        _New(s.iface, u"org.gnome.SessionManager"_q,
             u"/org/gnome/SessionManager"_q, u"org.gnome.SessionManager"_q);
        if (!(s.gnome = s.iface->isValid())) {
            _Debug("Failed to connect 'org.gnome.SessionManager'. "
                   "Fallback to 'org.freedesktop.ScreenSaver'.");
            _Renew(s.iface, u"org.freedesktop.ScreenSaver"_q,
                   u"/ScreenSaver"_q, u"org.freedesktop.ScreenSaver"_q);
            if (!s.iface->isValid()) {
                _Debug("Failed to connect 'org.freedesktop.ScreenSaver'. "
                       "Fallback to XCB_SCREEN_SAVER_RESET.");
                _Delete(s.iface);
                s.xss = true;
            }
        }
    }
    s.reset.stop();
    if (disabled) {
        if (s.iface) {
            if (s.gnome)
                s.reply = s.iface->call(u"Inhibit"_q, u"bomi"_q, 0u,
                                        u"Running player"_q, 4u | 8u);
            else
                s.reply = s.iface->call(u"Inhibit"_q, u"bomi"_q,
                                        u"Running player"_q);
            if (!s.reply.isValid()) {
                _Error("DBus '%%' error: %%", s.iface->interface(),
                       s.reply.error().message());
                _Error("Fallback to XCB_SCREEN_SAVER_RESET.");
                _Delete(s.iface);
                s.xss = true;
            } else
                _Debug("Disable screensaver with '%%'.", s.iface->interface());
        }
        if (s.xss) {
            xcb_screensaver_suspend(d->connection, 1);
            _Debug("Disable screensaver with XCB_SCREEN_SAVER_RESET.");
            s.reset.start();
        }
    } else {
        if (s.iface) {
            auto res = s.iface->call(s.gnome ? u"Uninhibit"_q : u"UnInhibit"_q,
                                     s.reply.value());
            if (res.type() == QDBusMessage::ErrorMessage)
                _Error("DBus '%%' error: [%%] %%", s.iface->interface(),
                       res.errorName(), res.errorMessage());
            else
                _Debug("Enable screensaver with '%%'.", s.iface->interface());
        }
        if (s.xss)
            xcb_screensaver_suspend(d->connection, 0);
    }
    s.inhibit = disabled;
}

auto isFullScreen(const QWidget *w) -> bool
{
    return w->isFullScreen();
}

auto setFullScreen(QWidget *widget, bool fs) -> void
{
    if (widget->isFullScreen() != fs)
        d->sendState(widget, fs, _NET_WM_STATE_FULLSCREEN);
}

auto isAlwaysOnTop(const QWidget *w) -> bool
{
    const auto cookie = xcb_get_property_unchecked
        (d->connection, 0, w->winId(), d->atoms[_NET_WM_STATE], XCB_ATOM_ATOM, 0, 1024);
    auto reply = _Reply(xcb_get_property_reply(d->connection, cookie, nullptr));
    if (!reply || reply->format != 32 || reply->type != XCB_ATOM_ATOM)
        return false;
    auto begin = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.data()));
    auto end = begin + reply->length;
    if (std::find(begin, end, d->atoms[_NET_WM_STATE_STAYS_ON_TOP]) != end)
        return true;
    if (std::find(begin, end, d->atoms[_NET_WM_STATE_ABOVE]) != end)
        return true;
    return false;
}

auto setAlwaysOnTop(QWidget *widget, bool on) -> void
{
    d->sendState(widget, on, _NET_WM_STATE_ABOVE, _NET_WM_STATE_STAYS_ON_TOP);
}

auto opticalDrives() -> QStringList
{
    static const QStringList filter = QStringList() << u"sr*"_q
        << u"sg*"_q << u"scd*"_q << u"dvd*"_q << u"cd*"_q;
    QDir dir(u"/dev"_q);
    QStringList devices;
    for (auto &dev : dir.entryList(filter, QDir::System))
        devices.append(u"/dev/"_q % dev);
    return devices;
}

auto canShutdown() -> bool
{
    // Can I check a dbus call is actually callable without calling it?
    return true;
}

auto shutdown() -> bool
{
    using Iface = QDBusInterface;
    auto bus = QDBusConnection::sessionBus();
    _Debug("Try KDE session manger to shutdown.");
    Iface kde(u"org.kde.ksmserver"_q, u"/KSMServer"_q,
              u"org.kde.KSMServerInterface"_q, bus);
    auto response = kde.call(u"logout"_q, 0, 2, 2);
    auto check = [&] (const char *fmt, const char *fb) -> bool {
        if (response.type() != QDBusMessage::ErrorMessage)
            return true;
        _Debug(fmt, response.errorName(), response.errorMessage());
        _Debug(fb);
        return false;
    };
    if (check("KDE session manager does not work: [%%] %%",
              "Fallback to Gnome session manager."))
        return true;
    Iface gnome(u"org.gnome.SessionManager"_q,
                u"/org/gnome/SessionManager"_q,
                u"org.gnome.SessionManager"_q, bus);
    response = gnome.call(u"RequestShutdown"_q);
    if (check("Gnome session manager does not work: [%%] %%",
              "Fallback to gnome-power-cmd.sh."))
        return true;
    if (QProcess::startDetached(u"gnome-power-cmd.sh shutdown"_q)
            || QProcess::startDetached(u"gnome-power-cmd shutdown"_q))
        return true;
    bus = QDBusConnection::systemBus();
    _Debug("gnome-power-cmd.sh does not work.");
    _Debug("Fallback to HAL.");
    Iface hal(u"org.freedesktop.Hal"_q,
              u"/org/freedesktop/Hal/devices/computer"_q,
              u"org.freedesktop.Hal.Device.SystemPowerManagement"_q, bus);
    response = hal.call(u"Shutdown"_q);
    if (check("HAL does not work: [%%] %%", "Fallback to ConsoleKit."))
        return true;
    Iface consoleKit(u"org.freedesktop.ConsoleKit"_q,
                     u"/org/freedesktop/ConsoleKit/Manager"_q,
                     u"org.freedesktop.ConsoleKit.Manager"_q, bus);
    response = consoleKit.call(u"Stop"_q);
    if (check("ConsoleKit does not work: [%%] %%",
              "Sorry, there's no way to shutdown."))
        return true;
    return false;
}

auto systemTime() -> quint64
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec*1000000u + t.tv_usec;
}

static int getField(const char *file, const char *field)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    static char buffer[BUFSIZ]; // should be called in GUI thread!!
    const auto fd = open(file, O_RDONLY);
    if (fd < 0)
        return 0;
    int len = ::read(fd, buffer, BUFSIZ);
    int ret = 0;
    if (len > 0) {
        buffer[len] = '\0';
        auto pos = strstr(buffer, field);
        if (pos) {
            pos += strlen(field);
            do {
                if (!isspace(*pos) && *pos != ':')
                    break;
            } while (*(++pos));
            sscanf(pos, "%d", &ret);
        }
    }
    close(fd);
    return ret;
}


auto processTime() -> quint64
{
    static char buffer[BUFSIZ];
    static const quint64 tick = sysconf(_SC_CLK_TCK);
    int pid, ppid, pgrp, session, tty_nr, tpgid; uint flags;
    unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime; char comm[256], state;
    const auto fd = open("/proc/self/stat", O_RDONLY);
    if (fd < 0)
        return 0;
    int len = ::read(fd, buffer, BUFSIZ);
    if (len > 0) {
        buffer[len] = '\0';
        len = sscanf(buffer
                     , "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
                     , &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
                     , &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
    }
    close(fd);
    return len > 0 ? quint64(utime + stime)*Q_UINT64_C(1000000)/tick : 0;
}

auto totalMemory() -> double
{
    return getField("/proc/meminfo", "MemTotal")*1000.0 / (1024 * 1024);
}

auto usingMemory() -> double
{
    return getField("/proc/self/status", "VmRSS")*1000.0 / (1024 * 1024);
}

}

#endif
