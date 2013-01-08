#include "app.hpp"
#include "rootmenu.hpp"
#include "pref.hpp"
#include "videorenderer.hpp"
#include "global.hpp"
#include "mrl.hpp"
#include "avmisc.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "recentinfo.hpp"
#include "hwaccel.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "subtitlerendereritem.hpp"

static bool checkOpenGL() {
	if (Q_LIKELY(QGLFormat::hasOpenGL() && (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0)))
		return true;
	QMessageBox::critical(0, "CMPlayer"
		, App::tr("CMPlayer needs OpenGL to render video. Your system has no OpenGL support. Exit CMPlayer."));
	return false;
}

Q_DECLARE_METATYPE(QTextOption::WrapMode);

int main(int argc, char **argv) {
	QApplication::setAttribute(Qt::AA_X11InitThreads);

	qRegisterMetaType<State>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	qmlRegisterType<VideoRendererItem>("CMPlayer", 1, 0, "VideoRenderer");
	qmlRegisterType<PlayInfoItem>("CMPlayer", 1, 0, "PlayInfo");
	qmlRegisterType<AvInfoObject>();
	qmlRegisterType<AvIoFormat>();
	qmlRegisterType<MediaInfoObject>();
	qmlRegisterType<SubtitleRendererItem>("CMPlayer", 1, 0, "SubtitleRenderer");

	qDebug() << "started app";
	App app(argc, argv);
	qDebug() << "app created";
	if (!checkOpenGL())
		return 1;
	const auto mrl = app.getMrlFromCommandLine();
	if (app.isUnique() && app.sendMessage("wakeUp")) {
		if (!mrl.isEmpty())
			app.sendMessage(_L("mrl ") % mrl.toString());
		return 0;
	}
	HwAccelInfo::obj = new HwAccelInfo;
	PlayEngine::obj = new PlayEngine;
	PlayEngine::obj->start();
	qDebug() << "engine created";
	qDebug() << "engine started";
	RootMenu::obj = new RootMenu;
	Pref::obj = new Pref;
	Pref::obj->load();
	RecentInfo::obj = new RecentInfo;
	qDebug() << "global objs created";
	while (!PlayEngine::obj->isInitialized())
		PlayEngine::msleep(1);
	MainWindow *mw = new MainWindow;
	qDebug() << "mw created";
	app.setMainWindow(mw);
	mw->show();
	if (!mrl.isEmpty())
		mw->openFromFileManager(mrl);
	const int ret = app.exec();
	mw->exit();
	qDebug() << "gui loop end";
	PlayEngine::obj->wait();
	qDebug() << "playing thread finished";
	mw->unplug();
	qDebug() << "unplugged";
	delete mw;
	qDebug() << "main window deleted";
	delete PlayEngine::obj;
	qDebug() << "engine deleted";
	delete RecentInfo::obj;
	qDebug() << "recent deleted";
	delete Pref::obj;
	qDebug() << "pref deleted";
	delete RootMenu::obj;
	qDebug() << "menu deleted";
	delete HwAccelInfo::obj;
	return ret;
}
