#include "app_x11.hpp"

#ifdef Q_OS_LINUX
#include "app.hpp"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow.h>
extern "C" {
#include <xcb/xcb.h>
#include <X11/Xlib.h>
}

struct XWindowInfo {
	XWindowInfo() {}
	XWindowInfo(QWindow *w) {
		window = w->winId();
		connection = static_cast<xcb_connection_t*>(cApp.platformNativeInterface()->nativeResourceForWindow("connection", w));
		display = static_cast<Display*>(cApp.platformNativeInterface()->nativeResourceForWindow("display", w));
		root = getRoot(connection, window);
		netWmStateAtom = getAtom(connection, "_NET_WM_STATE");
		netWmStateAboveAtom = getAtom(connection, "_NET_WM_STATE_ABOVE");
		netWmStateStaysOnTopAtom = getAtom(connection, "_NET_WM_STATE_STAYS_ON_TOP");
	}
	xcb_connection_t *connection = nullptr;
	xcb_window_t window = 0, root = 0;
	xcb_atom_t netWmStateAtom = 0, netWmStateAboveAtom = 0, netWmStateStaysOnTopAtom = 0;
	Display *display = nullptr;
	static xcb_atom_t getAtom(xcb_connection_t *conn, const char *name) {
		auto cookie = xcb_intern_atom(conn, 0, strlen(name), name);
		auto reply = xcb_intern_atom_reply(conn, cookie, nullptr);
		if (!reply)
			return 0;
		auto ret = reply->atom;
		free(reply);
		return ret;
	}
	static xcb_window_t getRoot(xcb_connection_t *conn, xcb_window_t window) {
		auto geo = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), nullptr);
		auto ret = geo->root;
		free(geo);
		return ret;
	}
};

struct AppX11::Data {
	QTimer ss_timer;
	XWindowInfo x;
	QDBusInterface *iface = nullptr;
	QDBusReply<uint> reply;
	bool inhibit = false;
	bool xss = false;
	bool gnome = false;
};

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
	d->ss_timer.setInterval(20000);
	connect(&d->ss_timer, &QTimer::timeout, [this] () {
		if (d->xss && d->x.display)
			XResetScreenSaver(d->x.display);
	});
}

extern void finalize_vaapi();

AppX11::~AppX11() {
	finalize_vaapi();
	delete d->iface;
	delete d;
}

void AppX11::setScreensaverDisabled(bool disabled) {
	if (d->inhibit == disabled)
		return;
	if (disabled) {
		if (!d->iface && !d->xss) {
			d->iface = new QDBusInterface("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager");
			if (!(d->gnome = d->iface->isValid())) {
				delete d->iface;
				d->iface = new QDBusInterface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver");
				if (!d->iface->isValid()) {
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
				qDebug() << "DBus failed:" << d->reply.error().message();
				qDebug() << "fallback to XResetScreenSaver()";
				delete d->iface;
				d->iface = nullptr;
				d->xss = true;
			} else
				qDebug() << "disable with" << d->iface->interface();
		}
		if (d->xss)
			d->ss_timer.start();
	} else {
		if (d->iface) {
			auto response = d->iface->call(d->gnome ? "Uninhibit" : "UnInhibit", d->reply.value());
			if (response.type() == QDBusMessage::ErrorMessage)
				qDebug() << response.errorName() << response.errorMessage();
			else
				qDebug() << "enable with" << d->iface->interface();
		} else if (d->xss)
			d->ss_timer.stop();
	}
	d->inhibit = disabled;
}

void AppX11::setAlwaysOnTop(QWindow *window, bool onTop) {
	if (d->x.window != window->winId())
		d->x = XWindowInfo(window);
	xcb_client_message_event_t event;
	memset(&event, 0, sizeof(event));
	event.response_type = XCB_CLIENT_MESSAGE;
	event.format = 32;
	event.window = d->x.window;
	event.type = d->x.netWmStateAtom;
	event.data.data32[0] = onTop ? 1 : 0;
	event.data.data32[1] = d->x.netWmStateAboveAtom;
	event.data.data32[2] = d->x.netWmStateStaysOnTopAtom;
	xcb_send_event(d->x.connection, 0, d->x.root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event);
	xcb_flush(d->x.connection);
}

QStringList AppX11::devices() const {
	return QStringList();
}

bool AppX11::shutdown() {
	qDebug() << "Start to try shutdown";
	QDBusInterface kde("org.kde.ksmserver", "/KSMServer", "org.kde.KSMServerInterface", QDBusConnection::sessionBus());
	auto response = kde.call("logout", 0, 2, 2);
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	qDebug() << "KDE session manager does not work!" << response.errorName() << ":" << response.errorMessage();
	QDBusInterface gnome("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", QDBusConnection::sessionBus());
	response = gnome.call("RequestShutdown");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	qDebug() << "Gnome session manager does not work!" << response.errorName() << ":" << response.errorMessage();
	if (QProcess::startDetached("gnome-power-cmd.sh shutdown") || QProcess::startDetached("gnome-power-cmd shutdown"))
		return true;
	qDebug() << "gnome-power-cmd does not work!";
	QDBusInterface hal("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/computer", "org.freedesktop.Hal.Device.SystemPowerManagement", QDBusConnection::systemBus());
	response = hal.call("Shutdown");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	qDebug() << "HAL does not work!";
	QDBusInterface consoleKit("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", QDBusConnection::systemBus());
	response = consoleKit.call("Stop");
	if (response.type() != QDBusMessage::ErrorMessage)
		return true;
	qDebug() << "ConsoleKit does not work!";
	qDebug() << "Sorry, there's nothing I can do.";
	return false;
}

#endif
