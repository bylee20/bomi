#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "videoformat.hpp"
#include "hwacc.hpp"
#include "openglcompat.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(Main)

int main(int argc, char **argv) {
	qputenv("PX_MODULE_PATH", "/this-is-dummy-path-to-disable-libproxy");
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale = (void(*)(void))QLibrary::resolve(_L("gtk-x11-2.0"), 0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	PlayEngine::registerObjects();
	App app(argc, argv);
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]")))
		_Fatal("Another instance of CMPlayer is already running. Exit this...");
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
