#include "app.hpp"
#include "playengine.hpp"
#include "events.hpp"
#include "mainwindow.hpp"
#include "mrl.hpp"
#include "record.hpp"
#include <QtGui/QFileOpenEvent>
#include <QtGui/QStyleFactory>
#include <QtGui/QStyle>
#include <QtCore/QUrl>
#include <QtGui/QMenuBar>
#include <QtCore/QDebug>

#if defined(Q_WS_MAC)
#include "app_mac.hpp"
#elif defined(Q_WS_X11)
#include "app_x11.hpp"
#endif
#include <unistd.h>

#define APP_GROUP QLatin1String("application")

struct App::Data {
	QStringList styleNames;
#ifdef Q_WS_MAC
	QMenuBar *mb = new QMenuBar;
#else
	QMenuBar *mb = nullptr;
#endif
//	QUrl url;
//	QProcess cpu;
	MainWindow *main = nullptr;
#if defined(Q_WS_MAC)
	AppMac helper;
#elif defined(Q_WS_X11)
	AppX11 helper;
#endif
};

App::App(int &argc, char **argv)
: QtSingleApplication("net.xylosper.CMPlayer", argc, argv), d(new Data) {
	setOrganizationName("xylosper");
	setOrganizationDomain("xylosper.net");
	setApplicationName("CMPlayer");
	setQuitOnLastWindowClosed(false);
#ifndef Q_WS_MAC
	setWindowIcon(defaultIcon());
#endif
	auto makeStyleNameList = [this] () {
		auto names = QStyleFactory::keys();
		const auto defaultName = style()->objectName();
		for (auto it = ++names.begin(); it != names.end(); ++it) {
			if (defaultName.compare(*it, Qt::CaseInsensitive) == 0) {
				const auto name = *it;
				names.erase(it);
				names.prepend(name);
				break;
			}
		}
		return names;
	};

	auto makeStyle = [this]() {
		Record r(APP_GROUP);
		const auto name = r.read("style", styleName());
		if (style()->objectName().compare(name, Qt::CaseInsensitive) == 0)
			return;
		if (!d->styleNames.contains(name, Qt::CaseInsensitive))
			return;
		setStyle(QStyleFactory::create(name));
	};

	d->styleNames = makeStyleNameList();
	makeStyle();
	connect(this, SIGNAL(messageReceived(QString)), this, SLOT(onMessageReceived(QString)));
}

App::~App() {
	delete d->mb;
	delete d;
}

void App::setMainWindow(MainWindow *mw) {
	d->main = mw;
	setActivationWindow(d->main, false);
}

MainWindow *App::mainWindow() const {
	return d->main;
}

QIcon App::defaultIcon() {
	static QIcon icon;
	static bool init = false;
	if (!init) {
		icon.addFile(":/img/cmplayer16.png", QSize(16, 16));
		icon.addFile(":/img/cmplayer22.png", QSize(22, 22));
		icon.addFile(":/img/cmplayer24.png", QSize(24, 24));
		icon.addFile(":/img/cmplayer32.png", QSize(32, 32));
		icon.addFile(":/img/cmplayer48.png", QSize(48, 48));
		icon.addFile(":/img/cmplayer64.png", QSize(64, 64));
		icon.addFile(":/img/cmplayer128.png", QSize(128, 128));
		icon.addFile(":/img/cmplayer256.png", QSize(256, 256));
		icon.addFile(":/img/cmplayer512.png", QSize(512, 512));
		init = true;
	}
	return icon;
}

void App::setAlwaysOnTop(QWidget *widget, bool onTop) {
	d->helper.setAlwaysOnTop(widget->effectiveWinId(), onTop);
}

void App::setScreensaverDisabled(bool disabled) {
	d->helper.setScreensaverDisabled(disabled);
}

bool App::event(QEvent *event) {
	switch ((int)event->type()) {
	case QEvent::FileOpen: {
		if (d->main)
			d->main->openMrl(Mrl(static_cast<QFileOpenEvent*>(event)->url().toString()));
		event->accept();
		return true;
	} case Event::Reopen:
		d->main->show();
		event->accept();
		return true;
	default:
		return QApplication::event(event);
	}
}

QStringList App::devices() const {
	return d->helper.devices();
}

#ifdef Q_WS_MAC
QMenuBar *App::globalMenuBar() const {
	return d->mb;
}
#endif

Mrl App::getMrlFromCommandLine() {
	const QStringList args = arguments();
	return args.size() > 1 ? Mrl(args.last()) : Mrl();
}

void App::open(const QString &mrl) {
	if (!mrl.isEmpty() && d->main)
		d->main->openMrl(mrl);
}

void App::onMessageReceived(const QString &message) {
	if (message == "wakeUp") {
		activateWindow();
	} else if (message.left(3) == "mrl") {
		auto open = [this] (const QString &mrl) {
			if (!mrl.isEmpty() && d->main)
				d->main->openMrl(mrl);
		};
		open(message.right(message.size()-4));
	}
}

QStringList App::availableStyleNames() const {
	return d->styleNames;
}

void App::setStyleName(const QString &name) {
	if (!d->styleNames.contains(name, Qt::CaseInsensitive))
		return;
	if (style()->objectName().compare(name, Qt::CaseInsensitive) == 0)
		return;
	setStyle(QStyleFactory::create(name));
	Record r(APP_GROUP);
	r.write("style", name);
}

void App::setUnique(bool unique) {
	Record r(APP_GROUP);
	r.write("unique", unique);
}

QString App::styleName() const {
	return style()->objectName();
}

bool App::isUnique() const {
	Record r(APP_GROUP);
	return r.read("unique", true);
}

#undef APP_GROUP

bool App::shutdown() {
	return d->helper.shutdown();
}





//#define AppX11 App
//void AppX11::shutdown() {
//	bool shutdown_works = false;
//	bool gnome_power1 = false;
//	bool gnome_power2 = false;
//	bool hal_works = false;
//	QDBusMessage response;

//	QDBusInterface gnomeSessionManager("org.gnome.SessionManager",
//	  "/org/gnome/SessionManager", "org.gnome.SessionManager",
//	  QDBusConnection::sessionBus());
//	response = gnomeSessionManager.call("RequestShutdown");
//	if(response.type() == QDBusMessage::ErrorMessage){
//	  if(verbose)
//		qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  gnome_power1 = QProcess::startDetached("gnome-power-cmd.sh shutdown");
//	  gnome_power2 = QProcess::startDetached("gnome-power-cmd shutdown");
//	  if(verbose && !gnome_power1 && !gnome_power2)
//		qWarning() << "W: gnome-power-cmd and gnome-power-cmd.sh didn't work";
//	}
//	else
//	  shutdown_works = true;

//	QDBusInterface kdeSessionManager("org.kde.ksmserver", "/KSMServer",
//	  "org.kde.KSMServerInterface", QDBusConnection::sessionBus());
//	response = kdeSessionManager.call("logout", 0, 2, 2);
//	if(response.type() == QDBusMessage::ErrorMessage){
//	  if(verbose)
//		qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	}
//	else
//	  shutdown_works = true;

//	if(!shutdown_works && !gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.Hal",
//		"/org/freedesktop/Hal/devices/computer",
//		"org.freedesktop.Hal.Device.SystemPowerManagement",
//		QDBusConnection::systemBus());
//	  response = powermanagement.call("Shutdown");
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  }
//	  else
//		hal_works = true;
//	}

//	if(!hal_works && !shutdown_works && !gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.ConsoleKit",
//		"/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager",
//		QDBusConnection::systemBus());
//	  response = powermanagement.call("Stop");
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//		QProcess::startDetached("sudo shutdown -P now");
//	  }
//	}}

//May 14, 2011
//Leon Leon
//Ant Farmer
//218 posts

//Ambassador
//Thread Master L2


//link

////REBOOT
//void Converter::reboot()
//{bool reboot_works = false;
//	bool gnome_power1 = false;
//	bool gnome_power2 = false;
//	bool hal_works = false;
//	QDBusMessage response;

//	QDBusInterface gnomeSessionManager("org.gnome.SessionManager",
//	  "/org/gnome/SessionManager", "org.gnome.SessionManager",
//	  QDBusConnection::sessionBus());
//	response = gnomeSessionManager.call("RequestReboot");
//	if(response.type() == QDBusMessage::ErrorMessage){
//	  if(verbose)
//		qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  gnome_power1 = QProcess::startDetached("gnome-power-cmd.sh reboot");
//	  gnome_power2 = QProcess::startDetached("gnome-power-cmd reboot");
//	  if(verbose && !gnome_power1 && !gnome_power2)
//		qWarning() << "W: gnome-power-cmd and gnome-power-cmd.sh didn't work";
//	}
//	else
//	  reboot_works = true;

//	QDBusInterface kdeSessionManager("org.kde.ksmserver", "/KSMServer",
//	  "org.kde.KSMServerInterface", QDBusConnection::sessionBus());
//	response = kdeSessionManager.call("logout", 0, 2, 1);
//	if(response.type() == QDBusMessage::ErrorMessage){
//	  if(verbose)
//		qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	}
//	else
//	  reboot_works = true;

//	if(!reboot_works && !gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.Hal",
//		"/org/freedesktop/Hal/devices/computer",
//		"org.freedesktop.Hal.Device.SystemPowerManagement",
//		QDBusConnection::systemBus());
//	  response = powermanagement.call("Reboot");
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  }
//	  else
//		hal_works = true;
//	}

//	if(!hal_works && !reboot_works && !gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.ConsoleKit",
//		"/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager",
//		QDBusConnection::systemBus());
//	  response = powermanagement.call("Restart");
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//		QProcess::startDetached("sudo shutdown -r now");
//	  }
//	}}

////HIBERNATE
//void Converter::hibernate()
//{bool gnome_power1 = false;
//	bool gnome_power2 = false;
//	bool hal_works = false;
//	QDBusMessage response;

//	gnome_power1 = QProcess::startDetached("gnome-power-cmd.sh hibernate");
//	gnome_power2 = QProcess::startDetached("gnome-power-cmd hibernate");
//	if(!gnome_power1 && !gnome_power2 && verbose)
//	  qWarning() << "W: gnome-power-cmd and gnome-power-cmd.sh didn't work";

//	if(!gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.Hal",
//		"/org/freedesktop/Hal/devices/computer",
//		"org.freedesktop.Hal.Device.SystemPowerManagement",
//		QDBusConnection::systemBus());
//	  response = powermanagement.call("Hibernate");
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  }
//	  else
//		hal_works = true;
//	}

//	if(!hal_works && !gnome_power1 && !gnome_power2){
//	  QDBusInterface powermanagement("org.freedesktop.DeviceKit.Power", "/org/freedesktop/DeviceKit/Power",
//		"org.freedesktop.DeviceKit.Power", QDBusConnection::systemBus());
//	  if(response.type() == QDBusMessage::ErrorMessage){
//		if(verbose)
//		  qWarning() << "W: " << response.errorName() << ":" << response.errorMessage();
//	  }
//	}}

////LOCK
//void Converter::lock()
//{
//		if(system("gnome-screensaver-command -l") && system("dbus-send --type=method_call --dest=org.gnome.ScreenSaver /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock"))
//			QMessageBox::warning(this, "Error", "Unable to lock computer.", QMessageBox::Ok, NULL);

//}



