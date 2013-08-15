#include "app.hpp"
#include "rootmenu.hpp"
#include "playengine.hpp"
#include "mainwindow.hpp"
#include "mrl.hpp"
#include "record.hpp"

#if defined(Q_OS_MAC)
#include "app_mac.hpp"
#elif defined(Q_OS_LINUX)
#include "app_x11.hpp"
#endif

#define APP_GROUP _L("application")

struct App::Data {
	QStringList styleNames;
#ifdef Q_OS_MAC
	QMenuBar *mb = new QMenuBar;
#else
	QMenuBar *mb = nullptr;
#endif
	MainWindow *main = nullptr;
#if defined(Q_OS_MAC)
	AppMac helper;
#elif defined(Q_OS_LINUX)
	AppX11 helper;
#endif
	LocalConnection connection = {"net.xylosper.CMPlayer", nullptr};
};

App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data) {
	connect(&d->connection, &LocalConnection::messageReceived, this, &App::messageReceived);

	//	actWin = 0;
	//	peer = new LocalConnection(_L("net.xylosper.CMPlayer"), this);
	//	connect(peer, SIGNAL(messageReceived(const QString&)), SIGNAL(messageReceived(const QString&)));

	setOrganizationName("xylosper");
	setOrganizationDomain("xylosper.net");
	setApplicationName("CMPlayer");
	setQuitOnLastWindowClosed(false);
//	setFont(QFont(QString::fromUtf8("나눔 고딕")));
#ifndef Q_OS_MAC
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
		auto name = r.value("style", styleName()).toString();
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
#ifndef Q_OS_MAC
	d->main->setWindowIcon(defaultIcon());
#endif
}

void App::setFileName(const QString &fileName) {
	if (d->main) {
		const QString title = fileName % _L(" - ") % applicationName();
		d->main->setWindowTitle(title);
#ifdef Q_OS_LINUX
		d->helper.setWmName(d->main->window()->windowHandle(), title);
#endif
	}
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
		icon.addFile(":/img/cmplayer-logo.png", QSize(512, 512));
		init = true;
	}
	return icon;
}

void App::setAlwaysOnTop(QWindow *window, bool onTop) {
	d->helper.setAlwaysOnTop(window, onTop);
}

void App::setScreensaverDisabled(bool disabled) {
	d->helper.setScreensaverDisabled(disabled);
}

bool App::event(QEvent *event) {
	switch ((int)event->type()) {
	case QEvent::FileOpen: {
		if (d->main)
			d->main->openFromFileManager(Mrl(static_cast<QFileOpenEvent*>(event)->url().toString()));
		event->accept();
		return true;
	} case ReopenEvent:
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

#ifdef Q_OS_MAC
QMenuBar *App::globalMenuBar() const {
	return d->mb;
}
#endif

Mrl App::getMrlFromCommandLine() {
	const auto args = parse(arguments());
	for (const Argument &arg : args) {
		if (arg.name == _L("open"))
			return Mrl(arg.value);
	}
	return Mrl();
}

Arguments App::parse(const QStringList &cmds) {
	Arguments args;
	for (QString cmd : cmds) {
		if (cmd.startsWith(_L("--")))
			args << Argument::fromCommand(cmd.mid(2));
	}
	return args;
}

void App::open(const QString &mrl) {
	if (!mrl.isEmpty() && d->main)
		d->main->openMrl(mrl);
}

void App::onMessageReceived(const QString &message) {
	const auto args = parse(message.split("[:sep:]"));
	for (const Argument &arg : args) {
		if (arg.name == _L("wake-up")) {
			d->main->setVisible(true);
			d->main->raise();
			d->main->activateWindow();
		} else if (arg.name == _L("open")) {
			const Mrl mrl(arg.value);
			if (!mrl.isEmpty() && d->main)
				d->main->openMrl(mrl);
		} else if (arg.name == _L("action")) {
			if (d->main) {
				const auto a = Argument::fromCommand(arg.value);
				RootMenu::execute(a.name, a.value);
			}
		}
	}
}

bool App::sendMessage(const QString &message, int timeout) {
	return d->connection.sendMessage(message, timeout);
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
	r.write(name, "style");
}

void App::setUnique(bool unique) {
	Record r(APP_GROUP);
	r.write(unique, "unique");
}

QString App::styleName() const {
	return style()->objectName();
}

bool App::isUnique() const {
	Record r(APP_GROUP);
	return r.value("unique", true).toBool();
}

#undef APP_GROUP

bool App::shutdown() {
	return d->helper.shutdown();
}

/**************************************************************************************/

#if defined(Q_OS_WIN)
#include <QtCore/QLibrary>
#include <QtCore/qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif
#if defined(Q_OS_UNIX)
#include <time.h>
#include <unistd.h>
#endif

static int getUid() {
#if defined(Q_OS_WIN)
	if (!pProcessIdToSessionId) {
		QLibrary lib("kernel32");
		pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
	}
	if (pProcessIdToSessionId) {
		DWORD sessionId = 0;
		pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
		return sessionId;
	}
#else
	return ::getuid();
#endif
}

struct LocalConnection::Data {
	QString id, socket;
	QLocalServer server;
	QLockFile *lock = nullptr;
};

constexpr static const char* ack = "ack";

LocalConnection::LocalConnection(const QString &id, QObject* parent)
: QObject(parent), d(new Data) {
	d->id = id;
	d->socket = id % '-' % QString::number(getUid(), 16);
	d->lock = new QLockFile(QDir::temp().path() % '/' % d->socket % "-lock");
	d->lock->setStaleLockTime(0);
}

LocalConnection::~LocalConnection() {
	delete d->lock;
	delete d;
}

bool LocalConnection::runServer() {
	if (d->lock->isLocked())
		return true;
	if (!d->lock->tryLock() && !(d->lock->removeStaleLockFile() && d->lock->tryLock()))
		return false;
	if (!d->server.listen(d->socket)) {
		QFile::remove(QDir::temp().path() % '/' % d->socket);
		if (!d->server.listen(d->socket))
			return false;
	}
	connect(&d->server, &QLocalServer::newConnection, [this]() {
		QScopedPointer<QLocalSocket> socket(d->server.nextPendingConnection());
		if (socket) {
			while (socket->bytesAvailable() < (int)sizeof(quint32))
				socket->waitForReadyRead();
			QDataStream in(socket.data());
			QByteArray msg; quint32 left;
			in >> left;
			msg.resize(left);
			char *buffer = msg.data();
			do {
				const int read = in.readRawData(buffer, left);
				if (read < 0)
					return;
				left -= read;
				buffer += read;
			} while (left > 0 && socket->waitForReadyRead(5000));
			QString message(QString::fromUtf8(msg));
			socket->write(ack, qstrlen(ack));
			socket->waitForBytesWritten(1000);
			socket->close();
			emit messageReceived(message); //### (might take a long time to return)
		}
	});
	return true;
}

bool LocalConnection::sendMessage(const QString &message, int timeout) {
	if (runServer())
		return false;
	QLocalSocket socket;
	socket.connectToServer(d->socket);
	if (!socket.waitForConnected(timeout))
		return false;
	const auto msg = message.toUtf8();
	QDataStream out(&socket);
	out.writeBytes(msg.constData(), msg.size());
	bool res = socket.waitForBytesWritten(timeout);
	res &= socket.waitForReadyRead(timeout);   // wait for ack
	res &= (socket.read(qstrlen(ack)) == ack);
	return res;
}
