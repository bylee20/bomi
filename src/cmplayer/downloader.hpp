#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include "stdafx.hpp"

//class QUrl;					class QIODevice;

class Downloader: public QObject {
	Q_OBJECT
public:
	Downloader(QObject *parent = nullptr);
	~Downloader();
	bool start(const QUrl &url);
	bool isRunning() const;
	QByteArray data() const;
	QUrl url() const;
signals:
	void downloaded(qint64 written, qint64 total);
	void finished();
	void started();
private:
	struct Data;
	Data *d;
};

#endif // DOWNLOADER_HPP
