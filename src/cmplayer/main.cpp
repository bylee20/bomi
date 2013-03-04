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

	App app(argc, argv);
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage("wakeUp")) {
		if (!mrl.isEmpty())
			app.sendMessage(_L("mrl ") % mrl.toString());
		return 0;
	}
	MainWindow mw;
	mw.show();
	app.setMainWindow(&mw);
	if (!mrl.isEmpty())
		mw.openFromFileManager(mrl);
	return app.exec();
}
