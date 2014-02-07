#include "downloader.hpp"

struct Downloader::Data {
	QUrl url;
	QNetworkAccessManager *nam = nullptr;
	bool running = false;
	QByteArray data;
};

Downloader::Downloader(QObject *parent)
: QObject(parent), d(new Data) {
	d->nam = new QNetworkAccessManager;
}

Downloader::~Downloader() {
	delete d->nam;
	delete d;
}

bool Downloader::start(const QUrl &url) {
	if (d->running)
		return false;
	d->url = url;
	d->running = true;
	emit started();
	auto reply = d->nam->get(QNetworkRequest(url));
	connect(reply, &QNetworkReply::downloadProgress, this, &Downloader::downloaded);
	connect(reply, &QNetworkReply::finished, [reply, this] () {
		d->data = reply->readAll();
		d->running = false;
		emit finished();
		reply->deleteLater();
	});
	return true;
}

QUrl Downloader::url() const {
	return d->url;
}

bool Downloader::isRunning() const {
	return d->running;
}

QByteArray Downloader::data() const {
	return d->data;
}

//void Downloader::downloadFinished(QNetworkReply *reply)
//{
//	QUrl url = reply->url();
//	if (reply->error()) {
//		fprintf(stderr, "Download of %s failed: %s\n",
//				url.toEncoded().constData(),
//				qPrintable(reply->errorString()));
//	} else {
//		QString filename = saveFileName(url);
//		if (saveToDisk(filename, reply))
//			printf("Download of %s succeeded (saved to %s)\n",
//				   url.toEncoded().constData(), qPrintable(filename));
//	}

//	d->currentDownloads.removeAll(reply);
//	reply->deleteLater();

//	if (d->currentDownloads.isEmpty())
//		// all downloads finished
//		QCoreApplication::instance()->quit();
//}


//QString Downloader::saveFileName(const QUrl &url)
//{
//	QString path = url.path();
//	QString basename = QFileInfo(path).fileName();

//	if (basename.isEmpty())
//		basename = "download";

//	if (QFile::exists(basename)) {
//		// already exists, don't overwrite
//		int i = 0;
//		basename += '.';
//		while (QFile::exists(basename + QString::number(i)))
//			++i;

//		basename += QString::number(i);
//	}

//	return basename;
//}

//bool Downloader::saveToDisk(const QString &filename, QIODevice *data)
//{
//	QFile file(filename);
//	if (!file.open(QIODevice::WriteOnly)) {
//		fprintf(stderr, "Could not open %s for writing: %s\n",
//				qPrintable(filename),
//				qPrintable(file.errorString()));
//		return false;
//	}

//	file.write(data->readAll());
//	file.close();

//	return true;
//}

//void Downloader::execute()
//{
//	QStringList args = QCoreApplication::instance()->arguments();
//	args.takeFirst();           // skip the first argument, which is the program's name
//	if (args.isEmpty()) {
//		printf("Qt Download example - downloads all URLs in parallel\n"
//			   "Usage: download url1 [url2... urlN]\n"
//			   "\n"
//			   "Downloads the URLs passed in the command-line to the local directory\n"
//			   "If the target file already exists, a .0, .1, .2, etc. is appended to\n"
//			   "differentiate.\n");
//		QCoreApplication::instance()->quit();
//		return;
//	}

//	foreach (QString arg, args) {
//		QUrl url = QUrl::fromEncoded(arg.toLocal8Bit());
//		doDownload(url);
//	}
//}

