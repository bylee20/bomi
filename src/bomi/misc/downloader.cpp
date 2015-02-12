#include "downloader.hpp"
#include <QQuickItem>
#include <QNetworkReply>

auto reg_downloader() -> void
{
    qmlRegisterType<Downloader>();
}

struct Downloader::Data {
    Downloader *p = nullptr;
    QUrl url;
    QNetworkAccessManager *nam = nullptr;
    bool running = false, canceled = false;
    QByteArray data;
    qint64 written = -1, total = -1;
    qreal rate = -1.0;
    QNetworkReply *reply = nullptr;
};

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , d(new Data)
{
    d->p = this;
    d->nam = new QNetworkAccessManager;
}

Downloader::~Downloader()
{
    if (d->reply)
        cancel();
    delete d->nam;
    delete d;
}

auto Downloader::writtenSize() const -> qint64
{
    return d->written;
}

auto Downloader::totalSize() const -> qint64
{
    return d->total;
}

auto Downloader::isCanceled() const -> bool
{
    return d->canceled;
}

auto Downloader::cancel() -> void
{
    if (d->reply) {
        d->canceled = true;
        d->reply->abort();
        if (d->reply)
            _Delete(d->reply);
        if (_Change(d->running, false))
            emit runningChanged();
        emit canceledChanged();
    }
}

auto Downloader::start(const QUrl &url) -> bool
{
    if (d->running)
        return false;
    if (_Change(d->url, url))
        emit urlChanged();
    d->running = true;
    if (_Change(d->canceled, false))
        emit canceledChanged();
    emit started();
    emit runningChanged();
    progress(-1, -1);
    d->reply = d->nam->get(QNetworkRequest(url));
    connect(d->reply, &QNetworkReply::downloadProgress,
            this, &Downloader::progress);
    connect(d->reply, &QNetworkReply::finished, [this] () {
        d->data = d->reply->readAll();
        d->running = false;
        emit finished();
        emit runningChanged();
        d->reply->deleteLater();
        d->reply = nullptr;
    });
    return true;
}

auto Downloader::progress(qint64 written, qint64 total) -> void
{
    if (_Change(d->total, total))
        emit totalSizeChanged(total);
    if (_Change(d->written, written))
        emit writtenSizeChanged(written);
    qreal rate = -1.0;
    if (total > 0)
        rate = written/(double)total;
    if (_Change(d->rate, rate))
        emit rateChanged();
    emit progressed(written, total);
}

auto Downloader::url() const -> QUrl
{
    return d->url;
}

auto Downloader::isRunning() const -> bool
{
    return d->running;
}

auto Downloader::rate() const -> qreal
{
    return d->rate;
}

auto Downloader::takeData() -> QByteArray
{
    auto data = d->data;
    d->data = QByteArray();
    return data;
}

auto Downloader::data() const -> QByteArray
{
    return d->data;
}
