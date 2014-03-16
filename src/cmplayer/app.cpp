#include "app.hpp"
#include "log.hpp"
#include "rootmenu.hpp"
#include "playengine.hpp"
#include "mainwindow.hpp"
#include "mrl.hpp"
#include "record.hpp"

#if defined(Q_OS_MAC)
#include "app_mac.hpp"
#elif defined(Q_OS_LINUX)
#include "app_x11.hpp"
#include "mpris.hpp"
#endif

#define APP_GROUP _L("application")

DECLARE_LOG_CONTEXT(App)

enum class LineCmd {
	Wake,
	Open,
	Action,
	LogLevel,
	OpenGLDebug,
	Debug
};

struct App::Data {
	Data(App *p): p(p) {}
	App *p = nullptr;
	bool gldebug = false;
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
	mpris::RootObject *mpris = nullptr;
#endif
	QCommandLineOption dummy{"__dummy__"};
	QCommandLineParser cmdParser, msgParser;
	QMap<LineCmd, QCommandLineOption> options;
	LocalConnection connection = {"net.xylosper.CMPlayer", nullptr};
	void addOption(LineCmd cmd, const char *name, const QString desc, const char *valueName = "", const QString &def = QString()) {
		addOption(cmd, QStringList() << _L(name), desc, valueName, def);
	}
	void addOption(LineCmd cmd, const QStringList &names, const QString desc, const char *valueName = "", const QString &def = QString()) {
		const QLatin1String vname = _L(valueName);
		if (desc.contains(_L("%1")))
			options.insert(cmd, QCommandLineOption(names, desc.arg(_L('<') % vname % _L('>')), vname, def));
		else
			options.insert(cmd, QCommandLineOption(names, desc, vname, def));
	}
	void execute(const QCommandLineParser *parser) {
		auto isSet = [parser, this] (LineCmd cmd) { return parser->isSet(options.value(cmd, dummy)); };
		auto value = [parser, this] (LineCmd cmd) { return parser->value(options.value(cmd, dummy)); };
		auto values = [parser, this] (LineCmd cmd) { return parser->values(options.value(cmd, dummy)); };
		if (isSet(LineCmd::LogLevel))
			Log::setMaximumLevel(value(LineCmd::LogLevel));
		if (isSet(LineCmd::OpenGLDebug) || qgetenv("CMPLAYER_GL_DEBUG").toInt())
			gldebug = true;
		if (main) {
			if (isSet(LineCmd::Wake))
				main->wake();
			Mrl mrl;
			if (isSet(LineCmd::Open))
				mrl = Mrl(value(LineCmd::Open));
			if (!parser->positionalArguments().isEmpty())
				mrl = Mrl(parser->positionalArguments().first());
			if (!mrl.isEmpty())
				main->openFromFileManager(mrl);
			const auto args = values(LineCmd::Action);
			if (!args.isEmpty())
				RootMenu::execute(args[0], args.value(1));
		}
		if (isSet(LineCmd::Debug)) {
			if (Log::maximumLevel() < Log::Debug)
				Log::setMaximumLevel(Log::Debug);
			gldebug = true;
			qputenv("CMPLAYER_MPV_VERBOSE", "v");
		}
	}
	QCommandLineParser *getCommandParser(QCommandLineParser *parser) const {
		for (auto it = options.begin(); it != options.end(); ++it)
			parser->addOption(*it);
		parser->addHelpOption();
		parser->addVersionOption();
		parser->addPositionalArgument("mrl", tr("The file path or URL to open."), "mrl");
		return parser;
	}
};


App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data(this)) {
#ifdef Q_OS_LINUX
	setlocale(LC_NUMERIC,"C");
#endif
	setOrganizationName("xylosper");
	setOrganizationDomain("xylosper.net");
	setApplicationName(Info::name());
	setApplicationVersion(Info::version());

	d->addOption(LineCmd::Open, "open", tr("Open given %1 for file path or URL."), "mrl");
	d->addOption(LineCmd::Wake, QStringList() << "wake", tr("Bring the application window in front."));
	d->addOption(LineCmd::Action, "action", tr("Exectute %1 action or open %1 menu."), "id");
	d->addOption(LineCmd::LogLevel, "log-level", tr("Maximum verbosity for log. %1 should be one of nexts:")
				 % "\n    " % Log::options().join(", "), "lv");
	d->addOption(LineCmd::OpenGLDebug, "opengl-debug", tr("Turn on OpenGL debug logger."));
	d->addOption(LineCmd::Debug, "debug", tr("Turn on options for debugging."));
	d->getCommandParser(&d->cmdParser)->process(arguments());
	d->getCommandParser(&d->msgParser);
	d->execute(&d->cmdParser);

#if defined(Q_OS_MAC) && defined(CMPLAYER_RELEASE)
	static const QByteArray path = QApplication::applicationDirPath().toLocal8Bit();
	_Debug("Set $LIBQUVI_SCRIPTSDIR=\"%%\"", QApplication::applicationDirPath());
	if (setenv("LIBQUVI_SCRIPTSDIR", path.data(), 1) < 0)
		_Error("Cannot set $LIBQUVI_SCRIPTSDIR. Some streaming functions won't work.");
#endif

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
		d->msgParser.parse(message.split("[:sep:]")); d->execute(&d->msgParser);
	});
}

App::~App() {
	setMprisActivated(false);
	delete d->main;
	delete d->mb;
	delete d;
}

bool App::isOpenGLDebugLoggerRequested() const {
	return d->gldebug;
}

void App::setMainWindow(MainWindow *mw) {
	d->main = mw;
#ifndef Q_OS_MAC
	d->main->setWindowIcon(defaultIcon());
#endif
}

void App::setWindowTitle(QWidget *widget, const QString &title) {
//	_Trace("Set window title of %% to '%%'.", widget->metaObject()->className(), title);
	const QString text = title % (title.isEmpty() ? "" : " - ") % applicationName();
	widget->setWindowTitle(text);
#ifdef Q_OS_LINUX
	d->helper.setWmName(widget, text);
#endif
}

void App::setMprisActivated(bool activated) {
#ifdef Q_OS_LINUX
	if (activated && !d->mpris)
		d->mpris = new mpris::RootObject(this);
	else if (!activated && d->mpris)
		_Delete(d->mpris);
#else
	Q_UNUSED(activated);
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
	d->execute(&d->cmdParser);
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
