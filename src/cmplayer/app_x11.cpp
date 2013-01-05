#include "app_x11.hpp"

#ifdef Q_WS_X11

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtCore/QProcess>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

struct AppX11::Data {
	QTimer *ss_timer;
};

AppX11::AppX11(QObject *parent)
: QObject(parent), d(new Data) {
	d->ss_timer = new QTimer;
	d->ss_timer->setInterval(10000);
	connect(d->ss_timer, SIGNAL(timeout()), this, SLOT(ss_reset()));
}

AppX11::~AppX11() {
	delete d->ss_timer;
	delete d;
}

void AppX11::setScreensaverDisabled(bool disabled) {
	if (disabled)
		d->ss_timer->start();
	else
		d->ss_timer->stop();
}

void AppX11::ss_reset() {
	XResetScreenSaver(QX11Info::display());
}

void AppX11::setAlwaysOnTop(WId wid, bool onTop) {
	XEvent e;
	memset(&e, 0, sizeof(e));
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(QX11Info::display(), "_NET_WM_STATE", False);
	e.xclient.display = QX11Info::display();
	e.xclient.window = wid;
	e.xclient.format = 32;
	e.xclient.data.l[0] = onTop ? 1 : 0;
	e.xclient.data.l[1] = XInternAtom(QX11Info::display(), "_NET_WM_STATE_ABOVE", False);
	e.xclient.data.l[2] = XInternAtom(QX11Info::display(), "_NET_WM_STATE_STAYS_ON_TOP", False);
	XSendEvent(QX11Info::display(), QX11Info::appRootWindow(), False, SubstructureRedirectMask, &e);
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
