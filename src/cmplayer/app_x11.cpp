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

static xcb_atom_t getAtom(xcb_connection_t *connection, const char *name) {
	auto cookie = xcb_intern_atom(connection, 0, strlen(name), name);
	auto reply = xcb_intern_atom_reply(connection, cookie, nullptr);
	if (!reply)
		return 0;
	auto ret = reply->atom;
	free(reply);
	return ret;
}

struct AppX11::Data {
	QTimer ss_timer;
	QDBusInterface *iface = nullptr;
	QDBusReply<uint> reply;
	bool inhibit = false;
	bool xss = false;
	bool gnome = false;
	QByteArray wmName;

	xcb_connection_t *connection = nullptr;
	xcb_window_t root = 0;
	xcb_atom_t aNetWmState = 0, aNetWmStateAbove = 0, aNetWmStateStaysOnTop = 0, aWmName = 0, aWmClass = 0;
	Display *display = nullptr;

	xcb_atom_t getAtom(const char *name) { return ::getAtom(connection, name); }
};

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
	d->ss_timer.setInterval(20000);
	connect(&d->ss_timer, &QTimer::timeout, [this] () {
		if (d->xss && d->display) {
			_Trace("Call XResetScreenSaver().");
			XResetScreenSaver(d->display);
		} else
			_Error("Cannot run XResetScreenSaver().");
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

void AppX11::setScreensaverDisabled(bool disabled) {
	if (d->inhibit == disabled)
		return;
	if (disabled) {
		if (!d->iface && !d->xss) {
			_Debug("Initialize screensaver functions.");
			_Debug("Try to connect 'org.gnome.SessionManager'.");
			d->iface = new QDBusInterface("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager");
			if (!(d->gnome = d->iface->isValid())) {
				_Debug("Failed to connect 'org.gnome.SessionManager'. Fallback to 'org.freedesktop.ScreenSaver'.");
				delete d->iface;
				d->iface = new QDBusInterface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver");
				if (!d->iface->isValid()) {
					_Debug("Failed to connect 'org.freedesktop.ScreenSaver'. Fallback to XResetScreenSaver().");
					delete d->iface;
					d->iface = nullptr;
					d->xss = true;
				}
			}
		}
		if (d->iface) {
			if (d->gnome)
				d->reply = d->iface->call("Inhibit", "CMPlayer", 0u, "Running player", 4u | 8u);
			else
				d->reply = d->iface->call("Inhibit", "CMPlayer", "Running player");
			if (!d->reply.isValid()) {
				_Error("DBus '%%' error: %%", d->iface->interface(), d->reply.error().message());
				_Error("Fallback to XResetScreenSaver().");
				delete d->iface;
				d->iface = nullptr;
				d->xss = true;
			} else
				_Debug("Disable screensaver with '%%'.", d->iface->interface());
		}
		if (d->xss) {
			_Debug("Disable screensaver with XResetScreenSaver().");
			d->ss_timer.start();
		}
	} else {
		if (d->iface) {
			auto response = d->iface->call(d->gnome ? "Uninhibit" : "UnInhibit", d->reply.value());
			if (response.type() == QDBusMessage::ErrorMessage)
				_Error("DBus '%%' error: [%%] %%", d->iface->interface(), response.errorName(), response.errorMessage());
			else
				_Debug("Enable screensaver with '%%'.", d->iface->interface());
		} else if (d->xss) {
			_Debug("Enable screensaver with XResetScreenSaver().");
			d->ss_timer.stop();
		}
	}
	d->inhibit = disabled;
}

void AppX11::setAlwaysOnTop(QWidget *widget, bool onTop) {
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
	xcb_send_event(d->connection, 0, d->root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event);
	xcb_flush(d->connection);
}

QStringList AppX11::devices() const {
	static const QStringList filter = QStringList() << _L("sr*")
		<< _L("sg*") << _L("scd*") << _L("dvd*") << _L("cd*");
	QDir dir("/dev");
	QStringList devices;
	for (auto &dev : dir.entryList(filter, QDir::System))
		devices.append(_L("/dev/") % dev);
	return devices;
}

void AppX11::setWmName(QWidget *widget, const QString &name) {
	d->wmName = name.toUtf8();
	char *utf8 = d->wmName.data();
	auto wid = widget->effectiveWinId();
	if (d->connection && d->display) {
		XTextProperty text;
		Xutf8TextListToTextProperty(d->display, &utf8, 1, XCompoundTextStyle, &text);
		XSetWMName(d->display, wid, &text);
//		xcb_icccm_set_wm_name(d->x.connection, d->x.window, XCB_ATOM_STRING, 8, d->wmName.size(), d->wmName.constData());
		const char className[] = "cmplayer\0CMPlayer";
		xcb_icccm_set_wm_class(d->connection, wid, sizeof(className), className);
	}
}

bool AppX11::shutdown() {
	_Debug("Try KDE session manger to shutdown.");
	QDBusInterface kde("org.kde.ksmserver", "/KSMServer", "org.kde.KSMServerInterface", QDBusConnection::sessionBus());
	auto response = kde.call("logout", 0, 2, 2);
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	_Debug("KDE session manager does not work: [%%] %%", response.errorName(), response.errorMessage());
	_Debug("Fallback to Gnome session manager.");
	QDBusInterface gnome("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", QDBusConnection::sessionBus());
	response = gnome.call("RequestShutdown");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	_Debug("Gnome session manager does not work: [%%] %%", response.errorName(), response.errorMessage());
	_Debug("Fallback to gnome-power-cmd.sh.");
	if (QProcess::startDetached("gnome-power-cmd.sh shutdown") || QProcess::startDetached("gnome-power-cmd shutdown"))
		return true;
	_Debug("gnome-power-cmd.sh does not work.");
	_Debug("Fallback to HAL.");
	QDBusInterface hal("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", QDBusConnection::systemBus());
	response = hal.call("Shutdown");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	_Debug("HAL does not work: [%%] %%", response.errorName(), response.errorMessage());
	_Debug("Fallback to ConsoleKit.");
	QDBusInterface consoleKit("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", QDBusConnection::systemBus());
	response = consoleKit.call("Stop");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	_Debug("ConsoleKit does not work: [%%] %%", response.errorName(), response.errorMessage());
	_Debug("Sorry, there's no way to shutdown.");
	return false;
}

#endif
