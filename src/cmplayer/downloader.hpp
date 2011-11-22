#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include <QtCore/QObject>

class QUrl;					class QIODevice;

class Downloader : public QObject {
	Q_OBJECT
public:
	Downloader();
	~Downloader();
	bool download(const QUrl &url, QIODevice *file, int timeout = -1);
	static bool get(const QUrl &url, QIODevice *file, int timeout = -1) {
		return Downloader().download(url, file, timeout);
	}
private slots:
	void slotTimeout();
	void slotHttpFinished(int id);
private:
	struct Data;
	Data *d;
};


#endif // DOWNLOADER_HPP
