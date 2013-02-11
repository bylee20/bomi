#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"

int main(int argc, char **argv) {
#ifdef QT_NO_FONTCONFIG
	qDebug() << "no fontconfig";
#endif


//	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qRegisterMetaType<EngineState>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	PlayerItem::registerItems();

	App app(argc, argv);
//	return 0;
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage("wakeUp")) {
		if (!mrl.isEmpty())
			app.sendMessage(_L("mrl ") % mrl.toString());
		return 0;
	}
//	QTimer::singleShot(5000, qApp, SLOT(quit()));
	MainWindow *mw = new MainWindow;
	app.setMainWindow(mw);
	mw->show();
	if (!mrl.isEmpty())
		mw->openFromFileManager(mrl);
	const int ret = app.exec();
	mw->exit();
	delete mw;
	return ret;
}
