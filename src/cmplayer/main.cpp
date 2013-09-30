#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

int main(int argc, char **argv) {
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qRegisterMetaType<EngineState>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	qRegisterMetaType<QVector<int>>("QVector<int>");
	PlayerItem::registerItems();
	qDebug() << "Create App instance";
	App app(argc, argv);
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]"))) {
		qDebug() << "Another instance is already running. Exit this.";
		return 0;
	}
	qDebug() << "Create MainWindow instance";
	MainWindow mw;
	mw.show();
	app.setMainWindow(&mw);
	qDebug() << "Start main event loop";
	return app.exec();
}
