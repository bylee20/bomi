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

enum class LineCmd {
	Unknown,
	WakeUp,
	Open,
	Action,
	Count
};

struct CmdInfo {
	CmdInfo(LineCmd cmd, const char *name, bool hasArg)
	: cmd(cmd), name(name), hasArg(hasArg) {}
	CmdInfo() {}
	LineCmd cmd = LineCmd::Unknown;
	const char *name = ""; // never nullptr
	bool hasArg = false;
};

static CmdInfo cmds[] = {
	{LineCmd::WakeUp, "wake-up", false},
	{LineCmd::Open, "open", true},
	{LineCmd::Action, "action", true}
};

struct Argument {
	LineCmd cmd = LineCmd::Unknown;
	QString value;
};

typedef QList<Argument> Arguments;

static Arguments parse(const QStringList &cmds) {
	auto fromCommand = [](const QStringList &cmds, int &cmdidx) {
		const QString cmd = cmds[cmdidx];
		if (!cmd.startsWith(_L("--")))
			return Argument();
		auto ref = cmd.midRef(2);
		for (const CmdInfo &info : ::cmds) {
			if (ref != _L(info.name))
				continue;
			Argument arg;
			arg.cmd = info.cmd;
			if (info.hasArg && ++cmdidx < cmds.size())
				arg.value = cmds[cmdidx];
			return arg;
		}
		return Argument();
	};
	Arguments args;
	for (int i=0; i<cmds.size(); ++i) {
		auto arg = fromCommand(cmds, i);
		if (arg.cmd == LineCmd::Unknown)
			qDebug() << "Not valid option:" << cmds[i];
		else
			args << arg;
	}
	return args;
}

struct App::Data {
	Data(App *p): p(p) {}
	App *p = nullptr;
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
	Arguments args() const {
		auto args = p->arguments();
		args.pop_front();
		return parse(args);
	}
	static QPair<QString, QString> splitEq(const QString &cmd) {
		QPair<QString,QString> pair;
		const int eq = cmd.indexOf('=');
		if (eq < 0)
			pair.first = cmd;
		else {
			pair.first = cmd.left(eq);
			pair.second = cmd.mid(eq+1);
		}
		return pair;
	}
	void execute(const Arguments &args) {
		for (const Argument &arg : args) {
			switch (arg.cmd) {
			case LineCmd::WakeUp:
				main->setVisible(true);
				main->raise();
				main->activateWindow();
				break;
			case LineCmd::Open: {
				const Mrl mrl(arg.value);
				if (!mrl.isEmpty() && main)
					main->openFromFileManager(mrl);
				break;
			} case LineCmd::Action:
				if (main) {
					const auto pair = splitEq(arg.value);
					RootMenu::execute(pair.first, pair.second);
				}
				break;
			default:
				break;
			}
		}
	}
};


App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data(this)) {
#ifdef Q_OS_LINUX
	setlocale(LC_NUMERIC,"C");
#endif
#if defined(Q_OS_MAC) && defined(CMPLAYER_RELEASE)
	static const QByteArray path = QApplication::applicationDirPath().toLocal8Bit();
	qDebug() << "set $LIBQUVI_SCRIPTSDIR:" << path;
	if (setenv("LIBQUVI_SCRIPTSDIR", path.data(), 1) < 0)
		qDebug() << "Cannot set $LIBQUVI_SCRIPTSDIR. Some streaming functions won't work.";
#endif
	setOrganizationName("xylosper");
	setOrganizationDomain("xylosper.net");
	setApplicationName("CMPlayer");
	setQuitOnLastWindowClosed(false);
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
	connect(&d->connection, &LocalConnection::messageReceived, [this] (const QString &message) {
		auto args = message.split("[:sep:]"); args.pop_front(); d->execute(parse(args));
	});
}

App::~App() {
	delete d->main;
	delete d->mb;
	delete d;
}

void App::setMainWindow(MainWindow *mw) {
	d->main = mw;
#ifndef Q_OS_MAC
	d->main->setWindowIcon(defaultIcon());
#endif
}

void App::setWindowTitle(QWidget *widget, const QString &title) {
	const QString text = title % (title.isEmpty() ? "" : " - ") % applicationName();
	widget->setWindowTitle(text);
#ifdef Q_OS_LINUX
	d->helper.setWmName(widget, text);
#endif
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

void App::setAlwaysOnTop(QWidget *widget, bool onTop) {
	d->helper.setAlwaysOnTop(widget, onTop);
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

void App::runCommands() {
	d->execute(d->args());
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
