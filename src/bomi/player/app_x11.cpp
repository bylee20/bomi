#include "app_x11.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(App)

#ifdef Q_OS_LINUX
#include "app.hpp"
#include "tmp/algorithm.hpp"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtX11Extras/QX11Info>

extern "C" {
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/screensaver.h>
}

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

struct AppX11::Data {
    QTimer xssTimer, hbTimer;
    QDBusInterface *iface = nullptr;
    QDBusReply<uint> reply;
    bool inhibit = false, xss = false, gnome = false;
    QString hbCommand;

    xcb_connection_t *connection = nullptr;
    xcb_window_t root = 0;
    Display *display = nullptr;

    xcb_atom_t aNetWmState = 0, aNetWmStateAbove = 0;
    xcb_atom_t aNetWmStateStaysOnTop = 0;


    xcb_atom_t atoms[XcbAtomEnd];

    auto getAtom(const char *name) -> xcb_atom_t
    { return ::getAtom(connection, name); }

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

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
    d->xssTimer.setInterval(20000);
    connect(&d->xssTimer, &QTimer::timeout, this, [this] () {
        if (d->xss && d->connection) {
            _Trace("Call xcb_force_screen_saver().");
            xcb_force_screen_saver(d->connection, XCB_SCREEN_SAVER_RESET);
            xcb_flush(d->connection);
        } else
            _Error("Cannot run xcb_force_screen_saver().");
    });
    connect(&d->hbTimer, &QTimer::timeout, this, [this] () {
        if (!d->hbCommand.isEmpty()) {
            if (QProcess::startDetached(d->hbCommand))
                _Trace("Run command: %%", d->hbCommand);
            else
                _Error("Cannot run command: %%", d->hbCommand);
        } else
            _Error("No command for heartbeat");
    });

    d->connection = QX11Info::connection();
    d->display = QX11Info::display();
    d->root = QX11Info::appRootWindow();

#define GET_ATOM(id) {d->atoms[id] = d->getAtom(#id);}
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

    d->aNetWmState = d->getAtom("_NET_WM_STATE");
    d->aNetWmStateAbove = d->getAtom("_NET_WM_STATE_ABOVE");
    d->aNetWmStateStaysOnTop = d->getAtom("_NET_WM_STATE_STAYS_ON_TOP");
}

AppX11::~AppX11() {
    delete d->iface;
    delete d;
}

template<class T>
static inline QSharedPointer<T> _Reply(T *t) { return QSharedPointer<T>(t, free); }

auto AppX11::refreshRate() const -> qreal
{
    auto sr = _Reply(xcb_randr_get_screen_resources_current_reply(
        d->connection,
        xcb_randr_get_screen_resources_current_unchecked(d->connection, d->root),
        nullptr));
    const auto len = xcb_randr_get_screen_resources_current_crtcs_length(sr.data());
    const int n = cApp.screenNumber();
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

auto AppX11::setHeartbeat(const QString &command, int interval) -> void
{
    d->hbCommand = command;
    d->hbTimer.setInterval(interval*1000);
}

auto AppX11::setScreensaverDisabled(bool disabled) -> void
{
    if (d->inhibit == disabled)
        return;
    const auto heartbeat = !d->hbCommand.isEmpty();
    if (!heartbeat && !d->iface && !d->xss) {
        _Debug("Initialize screensaver functions.");
        _Debug("Try to connect 'org.gnome.SessionManager'.");
        _New(d->iface, u"org.gnome.SessionManager"_q,
             u"/org/gnome/SessionManager"_q, u"org.gnome.SessionManager"_q);
        if (!(d->gnome = d->iface->isValid())) {
            _Debug("Failed to connect 'org.gnome.SessionManager'. "
                   "Fallback to 'org.freedesktop.ScreenSaver'.");
            _Renew(d->iface, u"org.freedesktop.ScreenSaver"_q,
                   u"/ScreenSaver"_q, u"org.freedesktop.ScreenSaver"_q);
            if (!d->iface->isValid()) {
                _Debug("Failed to connect 'org.freedesktop.ScreenSaver'. "
                       "Fallback to xcb_force_screen_saver().");
                _Delete(d->iface);
                d->xss = true;
            }
        }
    }
    d->xssTimer.stop();
    d->hbTimer.stop();
    if (disabled) {
        if (heartbeat) {
            _Debug("Disable screensaver with external command.");
            d->hbTimer.start();
        } else {
            if (d->iface) {
                if (d->gnome)
                    d->reply = d->iface->call(u"Inhibit"_q, u"bomi"_q, 0u,
                                              u"Running player"_q, 4u | 8u);
                else
                    d->reply = d->iface->call(u"Inhibit"_q, u"bomi"_q,
                                              u"Running player"_q);
                if (!d->reply.isValid()) {
                    _Error("DBus '%%' error: %%", d->iface->interface(),
                           d->reply.error().message());
                    _Error("Fallback to xcb_force_screen_saver().");
                    _Delete(d->iface);
                    d->xss = true;
                } else
                    _Debug("Disable screensaver with '%%'.",
                           d->iface->interface());
            }
            if (d->xss) {
                xcb_screensaver_suspend(d->connection, 1);
                _Debug("Disable screensaver with xcb_force_screen_saver().");
                d->xssTimer.start();
            }
        }
    } else {
        if (d->iface) {
            auto response = d->iface->call(d->gnome ? u"Uninhibit"_q
                                                    : u"UnInhibit"_q,
                                           d->reply.value());
            if (response.type() == QDBusMessage::ErrorMessage)
                _Error("DBus '%%' error: [%%] %%", d->iface->interface(),
                       response.errorName(), response.errorMessage());
            else
                _Debug("Enable screensaver with '%%'.", d->iface->interface());
        }
        if (d->xss) {
            xcb_screensaver_suspend(d->connection, 0);
            d->xssTimer.start();
        }
    }
    d->inhibit = disabled;
}


auto AppX11::setFullScreen(QWidget *widget, bool fs) -> void
{
    d->sendState(widget, fs, _NET_WM_STATE_FULLSCREEN);
}

auto AppX11::setAlwaysOnTop(QWidget *widget, bool on) -> void
{
    d->sendState(widget, on, _NET_WM_STATE_ABOVE, _NET_WM_STATE_STAYS_ON_TOP);
}

auto AppX11::devices() const -> QStringList
{
    static const QStringList filter = QStringList() << u"sr*"_q
        << u"sg*"_q << u"scd*"_q << u"dvd*"_q << u"cd*"_q;
    QDir dir(u"/dev"_q);
    QStringList devices;
    for (auto &dev : dir.entryList(filter, QDir::System))
        devices.append(u"/dev/"_q % dev);
    return devices;
}

auto AppX11::shutdown() -> bool
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

#endif
