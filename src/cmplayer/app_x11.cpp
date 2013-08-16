#include "app_x11.hpp"

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
//	XWindowInfo x;
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

extern void initialize_vaapi();
extern void finalize_vaapi();
extern void initialize_vdpau();
extern void finalize_vdpau();

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
	initialize_vaapi();
	initialize_vdpau();
	d->ss_timer.setInterval(20000);
	connect(&d->ss_timer, &QTimer::timeout, [this] () {
		if (d->xss && d->display)
			XResetScreenSaver(d->display);
	});
	d->connection = QX11Info::connection();
	d->display = QX11Info::display();
	d->root = QX11Info::appRootWindow();
	d->aNetWmState = d->getAtom("_NET_WM_STATE");
	d->aNetWmStateAbove = d->getAtom("_NET_WM_STATE_ABOVE");
	d->aNetWmStateStaysOnTop = d->getAtom("_NET_WM_STATE_STAYS_ON_TOP");
}

AppX11::~AppX11() {
	finalize_vaapi();
	finalize_vdpau();
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
	xcb_client_message_event_t event;
	memset(&event, 0, sizeof(event));
	event.response_type = XCB_CLIENT_MESSAGE;
	event.format = 32;
	event.window = window->winId();
	event.type = d->aNetWmState;
	event.data.data32[0] = onTop ? 1 : 0;
	event.data.data32[1] = d->aNetWmStateAbove;
	event.data.data32[2] = d->aNetWmStateStaysOnTop;
	xcb_send_event(d->connection, 0, d->root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event);
	xcb_flush(d->connection);
}

QStringList AppX11::devices() const {
	return QStringList();
}

void AppX11::setWmName(QWindow *window, const QString &name) {
	d->wmName = name.toUtf8();
	char *utf8 = d->wmName.data();
	auto wid = window->winId();
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
