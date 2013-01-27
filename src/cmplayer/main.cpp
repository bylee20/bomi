#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"
#include "resourcemonitor.hpp"
#include <sys/types.h>
#include <unistd.h>
//Q_DECLARE_METATYPE(QTextOption::WrapMode)

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
	qDebug() << ResourceMonitor::coreCount();
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
