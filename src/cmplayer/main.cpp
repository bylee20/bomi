#include "app.hpp"
#include "translator.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "videoformat.hpp"
#include "hwacc.hpp"
#include "openglcompat.hpp"
#include "log.hpp"
#include "tmp.hpp"
#include "udf25.hpp"

DECLARE_LOG_CONTEXT(Main)

QByteArray dvdHash(const QString &device) {
	static QStringList files = QStringList()
		<< _L("/VIDEO_TS/VIDEO_TS.IFO")
		<< _L("/VIDEO_TS/VTS_01_0.IFO")
		<< _L("/VIDEO_TS/VTS_02_0.IFO")
		<< _L("/VIDEO_TS/VTS_03_0.IFO")
		<< _L("/VIDEO_TS/VTS_04_0.IFO")
		<< _L("/VIDEO_TS/VTS_05_0.IFO")
		<< _L("/VIDEO_TS/VTS_06_0.IFO")
		<< _L("/VIDEO_TS/VTS_07_0.IFO")
		<< _L("/VIDEO_TS/VTS_08_0.IFO")
		<< _L("/VIDEO_TS/VTS_09_0.IFO");
	static constexpr int block = 2048;
	QByteArray data;
	if (QFileInfo(device).isDir()) {
		for (auto &fileName : files) {
			QFile file(device % fileName);
			if (!file.open(QFile::ReadOnly))
				break;
			data += file.read(block);
		}
	} else {
		udf::udf25 udf;
		if (!udf.Open(device.toLocal8Bit()))
			return QByteArray();
		for (auto &fileName : files) {
			::udf::File file(&udf, fileName);
			if (!file.isOpen())
				break;
			data += file.read(block);
		}
	}
	return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

QByteArray blurayHash(const QString &device) {
	static constexpr int block = 2048;
	QStringList files = QStringList() << _L("/BDMV/index.bdmv")
		<< _L("/BDMV/MovieObject.bdmv");
	QByteArray data;
	if (QFileInfo(device).isDir()) {
		auto dir = [&] (const QString &path) {
			QDir dir(device % path);
			auto list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
			const int count = qMin(5, list.size());
			for (int i=0; i<count; ++i)
				files.append(path % _L('/') % list[i]);
		};
		dir("/BDMV/PLAYLIST");
		dir("/BDMV/CLIPINF");
		dir("/BDMV/STREAM");
		qSort(files);
		for (auto &fileName : files) {
			QFile file(device % fileName);
			if (file.open(QFile::ReadOnly))
				data += file.read(block);
		}
	} else {
		udf::udf25 fs;
		if (!fs.Open(device.toLocal8Bit()))
			return QByteArray();
		auto dir = [&] (const QString &path) {
			::udf::Dir dir(&fs, path);
			const auto list = dir.files();
			const int count = qMin(5, list.size());
			for (int i=0; i<count; ++i)
				files.append(list[i]);
		};
		dir("/BDMV/PLAYLIST");
		dir("/BDMV/CLIPINF");
		dir("/BDMV/STREAM");
		qSort(files);
		for (auto &fileName : files) {
			::udf::File file(&fs, fileName);
			if (file.isOpen())
				data += file.read(block);
		}
	}
	return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

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
	if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]"))) {
		_Info("Another instance of CMPlayer is already running. Exit this...");
		return 0;
	}


//	qDebug() << dvd.open(QFile::ReadOnly);
//	qDebug() << dvd.read
//	dvd.close();
//	qDebug() << dvd.isReadable() << dvd.entryList(QDir::AllEntries);

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
