#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"

int main(int argc, char **argv) {
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qRegisterMetaType<EngineState>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	PlayerItem::registerItems();
	qDebug() << "Create App instance";
	App app(argc, argv);
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]")) {
//		if (!mrl.isEmpty())
//			app.sendMessage(_L("mrl ") % mrl.toString());
		qDebug() << "Another instance is already running. Exit this.";
		return 0;
	}
	qDebug() << "Create MainWindow instance";
	MainWindow mw;
	mw.show();
	app.setMainWindow(&mw);
	if (!mrl.isEmpty())
		mw.openFromFileManager(mrl);
	qDebug() << "Start main event loop";
	return app.exec();
}
