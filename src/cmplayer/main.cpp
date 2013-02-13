#include "app.hpp"
#include "mrl.hpp"
#include "mainwidget.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"

int main(int argc, char **argv) {
//	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qRegisterMetaType<EngineState>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	PlayerItem::registerItems();

	App app(argc, argv);
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage("wakeUp")) {
		if (!mrl.isEmpty())
			app.sendMessage(_L("mrl ") % mrl.toString());
		return 0;
	}
//	MainWidget *w = new MainWidget;	MainWindow *mw = w->mainWindow(); w->show();
	MainWindow *mw = new MainWindow;	mw->show();
	app.setMainWindow(mw);
	if (!mrl.isEmpty())
		mw->openFromFileManager(mrl);
	const int ret = app.exec();
	mw->exit();
	delete mw;
	return ret;
}
