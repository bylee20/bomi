#include "app_x11.hpp"
#include "misc/log.hpp"

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
#include <xcb/randr.h>
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

template<class T, class Free>
auto _MakeShared(T *t, Free f) { return QSharedPointer<T>(t, f); }

auto AppX11::refreshRate() const -> qreal
{
    auto sr = _MakeShared(xcb_randr_get_screen_resources_current_reply(
        d->connection,
        xcb_randr_get_screen_resources_current_unchecked(d->connection, d->root),
        nullptr), free);
    const auto len = xcb_randr_get_screen_resources_current_crtcs_length(sr.data());
    const int n = cApp.screenNumber();
    if (len < 1 || n >= len)
        return -1;
    auto crtc = xcb_randr_get_screen_resources_current_crtcs(sr.data())[n];
    auto ci = _MakeShared(xcb_randr_get_crtc_info_reply(
        d->connection,
        xcb_randr_get_crtc_info_unchecked(d->connection, crtc, XCB_CURRENT_TIME),
        nullptr), free);

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
                    d->reply = d->iface->call(u"Inhibit"_q, u"bomi"_q, 0u,
                                              u"Running player"_q, 4u | 8u);
                else
                    d->reply = d->iface->call(u"Inhibit"_q, u"bomi"_q,
                                              u"Running player"_q);
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
        auto response = d->iface->call(d->gnome ? u"Uninhibit"_q
                                                : u"UnInhibit"_q,
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
    static const QStringList filter = QStringList() << u"sr*"_q
        << u"sg*"_q << u"scd*"_q << u"dvd*"_q << u"cd*"_q;
    QDir dir(u"/dev"_q);
    QStringList devices;
    for (auto &dev : dir.entryList(filter, QDir::System))
        devices.append(u"/dev/"_q % dev);
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
    const char className[] = "bomi\0bomi";
    xcb_icccm_set_wm_class(d->connection, wid, sizeof(className), className);
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
        Log::write(Log::Debug, fmt, response.errorName(), response.errorMessage());
        Log::write(Log::Debug, fb);
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
