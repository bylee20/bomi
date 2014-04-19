#include "app.hpp"
#include "translator.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "videoformat.hpp"
#include "hwacc.hpp"
#include "openglcompat.hpp"
#include "log.hpp"
#include "tmp.hpp"
#include "globalqmlobject.hpp"
#include "historymodel.hpp"
#include "playlistmodel.hpp"
#include "downloader.hpp"
#include "quick/toplevelitem.hpp"
#include "quick/dialogitem.hpp"

DECLARE_LOG_CONTEXT(Main)

template<typename T>
static QObject *singletonProvider(QQmlEngine *, QJSEngine *) {
	return new T;
}

int main(int argc, char **argv) {
	qputenv("PX_MODULE_PATH", "/this-is-dummy-path-to-disable-libproxy");
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale = (void(*)(void))QLibrary::resolve(_L("gtk-x11-2.0"), 0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qmlRegisterType<PlaylistModel>();
	qmlRegisterType<HistoryModel>();
	qmlRegisterType<Downloader>();
	qmlRegisterType<TopLevelItem>();
	qmlRegisterType<ButtonBoxItem>("CMPlayer", 1, 0, "ButtonBox");
	qmlRegisterSingletonType<SettingsObject>("CMPlayer", 1, 0, "Settings", singletonProvider<SettingsObject>);
	qmlRegisterSingletonType<QmlApp>("CMPlayer", 1, 0, "App", singletonProvider<QmlApp>);
	PlayEngine::registerObjects();
	App app(argc, argv);
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]"))) {
		_Info("Another instance of CMPlayer is already running. Exit this...");
		return 0;
	}

	OpenGLCompat::check();
	HwAcc::initialize();
	MainWindow *mw = new MainWindow;
	_Debug("Show MainWindow.");
	mw->show();
	app.setMainWindow(mw);
	_Debug("Start main event loop.");
	auto ret = app.exec();
	HwAcc::finalize();
	_Debug("Exit...");
	return ret;
}
