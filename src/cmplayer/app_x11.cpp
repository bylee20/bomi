#include "app_x11.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(App)

#ifdef Q_OS_LINUX
#include "app.hpp"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtX11Extras/QX11Info>
extern "C" {
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_util.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

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
    QByteArray wmName;
    QString hbCommand;

    xcb_connection_t *connection = nullptr;
    xcb_window_t root = 0;
    xcb_atom_t aNetWmState = 0, aNetWmStateAbove = 0;
    xcb_atom_t aNetWmStateStaysOnTop = 0, aWmName = 0, aWmClass = 0;
    Display *display = nullptr;

    auto getAtom(const char *name) -> xcb_atom_t
    { return ::getAtom(connection, name); }
};

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
    d->xssTimer.setInterval(20000);
    connect(&d->xssTimer, &QTimer::timeout, this, [this] () {
        if (d->xss && d->display) {
            _Trace("Call XResetScreenSaver().");
            XResetScreenSaver(d->display);
        } else
            _Error("Cannot run XResetScreenSaver().");
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
    d->aNetWmState = d->getAtom("_NET_WM_STATE");
    d->aNetWmStateAbove = d->getAtom("_NET_WM_STATE_ABOVE");
    d->aNetWmStateStaysOnTop = d->getAtom("_NET_WM_STATE_STAYS_ON_TOP");
}

AppX11::~AppX11() {
    delete d->iface;
    delete d;
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
        _New(d->iface, "org.gnome.SessionManager",
             "/org/gnome/SessionManager", "org.gnome.SessionManager");
        if (!(d->gnome = d->iface->isValid())) {
            _Debug("Failed to connect 'org.gnome.SessionManager'. "
                   "Fallback to 'org.freedesktop.ScreenSaver'.");
            _Renew(d->iface, "org.freedesktop.ScreenSaver",
                   "/ScreenSaver", "org.freedesktop.ScreenSaver");
            if (!d->iface->isValid()) {
                _Debug("Failed to connect 'org.freedesktop.ScreenSaver'. "
                       "Fallback to XResetScreenSaver().");
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
                    d->reply = d->iface->call("Inhibit", "CMPlayer", 0u,
                                              "Running player", 4u | 8u);
                else
                    d->reply = d->iface->call("Inhibit", "CMPlayer",
                                              "Running player");
                if (!d->reply.isValid()) {
                    _Error("DBus '%%' error: %%", d->iface->interface(),
                           d->reply.error().message());
                    _Error("Fallback to XResetScreenSaver().");
                    _Delete(d->iface);
                    d->xss = true;
                } else
                    _Debug("Disable screensaver with '%%'.",
                           d->iface->interface());
            }
            if (d->xss) {
                _Debug("Disable screensaver with XResetScreenSaver().");
                d->xssTimer.start();
            }
        }
    } else if (d->iface) {
        auto response = d->iface->call(d->gnome ? "Uninhibit" : "UnInhibit",
                                       d->reply.value());
        if (response.type() == QDBusMessage::ErrorMessage)
            _Error("DBus '%%' error: [%%] %%", d->iface->interface(),
                   response.errorName(), response.errorMessage());
        else
            _Debug("Enable screensaver with '%%'.", d->iface->interface());
    }
    d->inhibit = disabled;
}

auto AppX11::setAlwaysOnTop(QWidget *widget, bool onTop) -> void
{
    if (!widget)
        return;
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));
    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = widget->winId();
    event.type = d->aNetWmState;
    event.data.data32[0] = onTop ? 1 : 0;
    event.data.data32[1] = d->aNetWmStateAbove;
    event.data.data32[2] = d->aNetWmStateStaysOnTop;
    const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                      | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    xcb_send_event(d->connection, 0, d->root, mask, (const char *)&event);
    xcb_flush(d->connection);
}

auto AppX11::devices() const -> QStringList
{
    static const QStringList filter = QStringList() << _L("sr*")
        << _L("sg*") << _L("scd*") << _L("dvd*") << _L("cd*");
    QDir dir("/dev");
    QStringList devices;
    for (auto &dev : dir.entryList(filter, QDir::System))
        devices.append(_L("/dev/") % dev);
    return devices;
}

auto AppX11::setWmName(QWidget *widget, const QString &name) -> void
{
    d->wmName = name.toUtf8();
    char *utf8 = d->wmName.data();
    auto wid = widget->effectiveWinId();
    if (!d->connection || !d->display)
        return;
    XTextProperty text;
    Xutf8TextListToTextProperty(d->display, &utf8, 1, XCompoundTextStyle, &text);
    XSetWMName(d->display, wid, &text);
//        xcb_icccm_set_wm_name(d->x.connection, d->x.window,
        //XCB_ATOM_STRING, 8, d->wmName.size(), d->wmName.constData());
    const char className[] = "cmplayer\0CMPlayer";
    xcb_icccm_set_wm_class(d->connection, wid, sizeof(className), className);
}

auto AppX11::shutdown() -> bool
{
    using Iface = QDBusInterface;
    auto bus = QDBusConnection::sessionBus();
    _Debug("Try KDE session manger to shutdown.");
    Iface kde("org.kde.ksmserver", "/KSMServer",
              "org.kde.KSMServerInterface", bus);
    auto response = kde.call("logout", 0, 2, 2);
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
    Iface gnome("org.gnome.SessionManager", "/org/gnome/SessionManager",
                "org.gnome.SessionManager", bus);
    response = gnome.call("RequestShutdown");
    if (check("Gnome session manager does not work: [%%] %%",
              "Fallback to gnome-power-cmd.sh."))
        return true;
    if (QProcess::startDetached("gnome-power-cmd.sh shutdown")
            || QProcess::startDetached("gnome-power-cmd shutdown"))
        return true;
    bus = QDBusConnection::systemBus();
    _Debug("gnome-power-cmd.sh does not work.");
    _Debug("Fallback to HAL.");
    Iface hal("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer",
              "org.freedesktop.Hal.Device.SystemPowerManagement", bus);
    response = hal.call("Shutdown");
    if (check("HAL does not work: [%%] %%",
              "Fallback to ConsoleKit."))
        return true;
    Iface consoleKit("org.freedesktop.ConsoleKit",
                     "/org/freedesktop/ConsoleKit/Manager",
                     "org.freedesktop.ConsoleKit.Manager", bus);
    response = consoleKit.call("Stop");
    if (check("ConsoleKit does not work: [%%] %%",
              "Sorry, there's no way to shutdown."))
        return true;
    return false;
}

#endif
