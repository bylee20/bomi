#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

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
    auto type(const QUrl &url, int timeout = 30000) -> QString;
    auto start(const QUrl &url, const QStringList &extFilter = QStringList()) -> bool;
    auto suffixes() const -> QStringList;
    auto isRunning() const -> bool;
    auto data() const -> QByteArray;
    auto takeData() -> QByteArray;
    auto url() const -> QUrl;
    auto totalSize() const -> qint64;
    auto writtenSize() const -> qint64;
    auto rate() const -> qreal;
    auto isCanceled() const -> bool;
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
    auto progress(qint64 written, qint64 total) -> void;
    struct Data;
    Data *d;
};

#endif // DOWNLOADER_HPP
