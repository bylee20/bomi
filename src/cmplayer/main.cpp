#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "hwaccel.hpp"
#include "playeritem.hpp"
#include "videoformat.hpp"

static bool checkOpenGL() {
	if (Q_LIKELY(QGLFormat::hasOpenGL() && (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0)))
		return true;
	QMessageBox::critical(0, "CMPlayer"
		, App::tr("CMPlayer needs OpenGL to render video. Your system has no OpenGL support. Exit CMPlayer."));
	return false;
}

Q_DECLARE_METATYPE(QTextOption::WrapMode)

int main(int argc, char **argv) {
	QApplication::setAttribute(Qt::AA_X11InitThreads);


	qRegisterMetaType<EngineState>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	PlayerItem::registerItems();

	App app(argc, argv);
	if (!checkOpenGL())
		return 1;
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage("wakeUp")) {
		if (!mrl.isEmpty())
			app.sendMessage(_L("mrl ") % mrl.toString());
		return 0;
	}
	MainWindow *mw = new MainWindow;
	app.setMainWindow(mw);
	mw->show();
	if (!mrl.isEmpty())
		mw->openFromFileManager(mrl);
	const int ret = app.exec();
	mw->exit();
	delete mw;
	HwAccelInfo::finalize();
	return ret;
}
