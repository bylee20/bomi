#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "videoformat.hpp"
#include "hwacc.hpp"
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

int main(int argc, char **argv) {
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	PlayEngine::registerObjects();
	qDebug() << "Create App instance";
	App app(argc, argv);
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]"))) {
		qDebug() << "Another instance is already running. Exit this.";
		return 0;
	}
	qDebug() << "Create MainWindow instance";
	HwAcc::initialize();
	MainWindow *mw = new MainWindow;
	mw->show();
	app.setMainWindow(mw);
	qDebug() << "Start main event loop";
	auto ret = app.exec();
	HwAcc::finalize();
	qDebug() << "Exit...";
	return ret;
}
