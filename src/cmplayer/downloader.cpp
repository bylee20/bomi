#include "downloader.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QEventLoop>
#include <QtNetwork/QHttp>
#include <QtCore/QTimer>

struct Downloader::Data {
	QHttp *http;
	int id;
	QEventLoop loop;
	QTimer timer;
};

Downloader::Downloader()
: d(new Data) {
	d->http = new QHttp(this);
	d->id = -1;
	d->timer.setSingleShot(true);
	connect(d->http, SIGNAL(requestFinished(int, bool))
			, this, SLOT(slotHttpFinished(int)));
	connect(&d->timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
}


Downloader::~Downloader() {
	delete d;
}

bool Downloader::download(const QUrl &url, QFile *file, int timeout) {
	const bool open = file->isOpen();
	if (!open && !file->open(QFile::WriteOnly))
			return false;
	const QString scheme = url.scheme().toLower();
	QHttp::ConnectionMode mode = QHttp::ConnectionModeHttp;
	if (scheme == "https")
		mode = QHttp::ConnectionModeHttps;
	else if (scheme != "http")
		return false;
	d->http->setHost(url.host(), mode, url.port() == -1 ? 0 : url.port());
	if (!url.userName().isEmpty())
		d->http->setUser(url.userName(), url.password());
	const QByteArray path = QUrl::toPercentEncoding(url.path(), "!$&'()*+,;=:@/");
	d->id = d->http->get(path, file);
	if (timeout != -1)
		d->timer.start(timeout);
	d->loop.exec();
	if (!open)
		file->close();
	return (d->http->error() == QHttp::NoError);
}



void Downloader::slotHttpFinished(int id) {
	if (id == d->id) {
		d->timer.stop();
		d->loop.quit();
	}
}

void Downloader::slotTimeout() {
	d->http->abort();
	d->loop.quit();
}
