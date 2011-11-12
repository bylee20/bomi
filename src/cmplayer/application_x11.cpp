#include "application_x11.hpp"

#ifdef Q_WS_X11

#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

struct ApplicationX11::Data {
	QTimer *ss_timer;
};

ApplicationX11::ApplicationX11(QObject *parent)
: QObject(parent), d(new Data) {
	d->ss_timer = new QTimer;
	d->ss_timer->setInterval(10000);
	connect(d->ss_timer, SIGNAL(timeout()), this, SLOT(ss_reset()));
}

ApplicationX11::~ApplicationX11() {
	delete d->ss_timer;
	delete d;
}

void ApplicationX11::setScreensaverDisabled(bool disabled) {
	if (disabled)
		d->ss_timer->start();
	else
		d->ss_timer->stop();
}

void ApplicationX11::ss_reset() {
	XResetScreenSaver(QX11Info::display());
}

void ApplicationX11::setAlwaysOnTop(WId wid, bool onTop) {
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

QStringList ApplicationX11::devices() const {
	return QStringList();
}

#endif
