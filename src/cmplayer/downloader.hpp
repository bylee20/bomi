#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include "stdafx.hpp"

//class QUrl;					class QIODevice;

class Downloader: public QObject {
	Q_OBJECT
	Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
	Q_PROPERTY(bool canceled READ isCanceled NOTIFY canceledChanged)
	Q_PROPERTY(qint64 totalSize READ totalSize NOTIFY totalSizeChanged)
	Q_PROPERTY(qint64 writtenSize READ writtenSize NOTIFY writtenSizeChanged)
	Q_PROPERTY(qreal rate READ rate NOTIFY rateChanged)
	Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)
public:
	Downloader(QObject *parent = nullptr);
	~Downloader();
	bool start(const QUrl &url);
	bool isRunning() const;
	QByteArray data() const;
	QByteArray takeData();
	QUrl url() const;
	qint64 totalSize() const;
	qint64 writtenSize() const;
	qreal rate() const;
	bool isCanceled() const;
	Q_INVOKABLE void cancel();
signals:
	void writtenSizeChanged(qint64 writtenSize);
	void totalSizeChanged(qint64 totalSize);
	void progressed(qint64 written, qint64 total);
	void rateChanged();
	void runningChanged();
	void finished();
	void started();
	void urlChanged();
	void canceledChanged();
private:
	void progress(qint64 written, qint64 total);
	struct Data;
	Data *d;
};

#endif // DOWNLOADER_HPP
