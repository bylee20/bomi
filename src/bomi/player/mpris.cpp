#include "mpris.hpp"
#include "playengine.hpp"
#include "playlistmodel.hpp"
#include "app.hpp"
#include "mainwindow.hpp"
#include "player/avinfoobject.hpp"
#include "misc/dataevent.hpp"

namespace mpris {

RootObject::RootObject(QObject *parent)
    : QObject(parent)
{
    m_name = u"org.mpris.MediaPlayer2.bomi"_q;
    auto bus = QDBusConnection::sessionBus();
    auto ok = bus.registerService(m_name);
    if (!ok) {
        m_name = m_name % ".instance"_a
                 % QString::number(qApp->applicationPid());
        ok = bus.registerService(m_name);
    }
    if (!ok)
        m_name.clear();
    else {
        m_mp2 = new MediaPlayer2(this);
        m_player = new Player(this);
        bus.registerObject(u"/org/mpris/MediaPlayer2"_q, this,
                           QDBusConnection::ExportAdaptors);
    }
}

RootObject::~RootObject()
{
    if (m_name.isEmpty())
        return;
    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(u"/org/mpris/MediaPlayer2"_q);
    bus.unregisterService(m_name);
}


static auto dbusTrackId(const Mrl &mrl) -> QString
{
    using Hash = QCryptographicHash;
    const auto hash = Hash::hash(mrl.toString().toUtf8(), Hash::Md5);
    return "/net/xylosper/bomi/track_"_a % _L(hash.toHex().constData());
}

static auto sendPropertiesChanged(const QDBusAbstractAdaptor *adaptor,
                                  const QVariantMap &properties) -> void
{
    const auto iface = _L(adaptor->metaObject()->classInfo(0).value());
    auto sig = QDBusMessage::createSignal(u"/org/mpris/MediaPlayer2"_q,
                                          u"org.freedesktop.DBus.Properties"_q,
                                          u"PropertiesChanged"_q);
    sig << iface << properties << QStringList();
    QDBusConnection::sessionBus().send(sig);
}

SIA sendPropertiesChanged(const QDBusAbstractAdaptor *adaptor,
                          const char *property, const QVariant &value) -> void
{
    QVariantMap props;
    props.insert(_L(property), value);
    sendPropertiesChanged(adaptor, props);
}

struct MediaPlayer2::Data {
    MainWindow *mw = nullptr;
};

MediaPlayer2::MediaPlayer2(QObject *parent)
: QDBusAbstractAdaptor(parent), d(new Data) {
    d->mw = cApp.mainWindow();
    Q_ASSERT(d->mw);
    connect(d->mw, &MainWindow::fullscreenChanged,
            [this] (bool fs) { sendPropertiesChanged(this, "Fullscreen", fs); });
}

MediaPlayer2::~MediaPlayer2() {
    delete d;
}

auto MediaPlayer2::isFullScreen() const -> bool
{
    return d->mw->isFullScreen();
}

auto MediaPlayer2::setFullScreen(bool fs) -> void
{
    d->mw->setFullScreen(fs);
}

auto MediaPlayer2::Raise() -> void
{
    d->mw->wake();
}

auto MediaPlayer2::Quit() -> void
{
    d->mw->exit();
}

auto MediaPlayer2::supportedUriSchemes() const -> QStringList
{
    return QStringList() << u"file"_q << u"http"_q << u"https"_q << u"ftp"_q
                         << u"mms"_q << u"rtmp"_q << u"rtsp"_q << u"sftp"_q;
}

auto MediaPlayer2::supportedMimeTypes() const -> QStringList
{
    return QStringList() << u"application/ogg"_q << u"application/x-ogg"_q
                         << u"application/sdp"_q << u"application/smil"_q
                         << u"application/x-smil"_q
                         << u"application/streamingmedia"_q
                         << u"application/x-streamingmedia"_q
                         << u"application/vnd.rn-realmedia"_q
                         << u"application/vnd.rn-realmedia-vbr"_q
                         << u"audio/aac"_q << u"audio/x-aac"_q
                         << u"audio/m4a"_q << u"audio/x-m4a"_q
                         << u"audio/mp1"_q << u"audio/x-mp1"_q
                         << u"audio/mp2"_q << u"audio/x-mp2"_q
                         << u"audio/mp3"_q << u"audio/x-mp3"_q
                         << u"audio/mpeg"_q << u"audio/x-mpeg"_q
                         << u"audio/mpegurl"_q << u"audio/x-mpegurl"_q
                         << u"audio/mpg"_q << u"audio/x-mpg"_q
                         << u"audio/rn-mpeg"_q << u"audio/scpls"_q
                         << u"audio/x-scpls"_q << u"audio/vnd.rn-realaudio"_q
                         << u"audio/wav"_q << u"audio/x-pn-windows-pcm"_q
                         << u"audio/x-realaudio"_q << u"audio/x-pn-realaudio"_q
                         << u"audio/x-ms-wma"_q << u"audio/x-pls"_q
                         << u"audio/x-wav"_q << u"video/mpeg"_q
                         << u"video/x-mpeg"_q << u"video/x-mpeg2"_q
                         << u"video/mp4"_q << u"video/msvideo"_q
                         << u"video/x-msvideo"_q << u"video/quicktime"_q
                         << u"video/vnd.rn-realvideo"_q << u"video/x-ms-afs"_q
                         << u"video/x-ms-asf"_q << u"video/x-ms-wmv"_q
                         << u"video/x-ms-wmx"_q << u"video/x-ms-wvxvideo"_q
                         << u"video/x-avi"_q << u"video/x-fli"_q
                         << u"video/x-flv"_q << u"video/x-theora"_q
                         << u"video/x-matroska"_q << u"video/mp2t"_q;
}

/******************************************************************************/

enum {
    SaveArtAlbum = QEvent::User + 1,
    UpdateAlbumArt
};

class Thread : public QThread {
public:
    Thread() { }
    Player *p = nullptr;
    auto saveAlbumArt(const QImage &image) -> void
        { _PostEvent(this, SaveArtAlbum, image); }
private:
    auto customEvent(QEvent *event) -> void final
    {
        switch ((int)event->type()) {
        case SaveArtAlbum: {
            auto img = _MoveData<QImage>(event);
            if (img.isNull())
                _PostEvent(p, UpdateAlbumArt, QString());
            if (img.width() > 512 || img.height() > 512)
                img = img.scaled(512, 512, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            Q_ASSERT(!m_albumArt[0]);
            m_albumArt[0].reset(new QTemporaryFile(QDir::tempPath() % u"/bomi-albumart-XXXXXX.png"_q));
            m_albumArt[0]->open();
            m_albumArt[0]->close();
            if (!img.save(m_albumArt[0]->fileName()))
                _PostEvent(p, UpdateAlbumArt, QString());
            else
                _PostEvent(p, UpdateAlbumArt, m_albumArt[0]->fileName());
            m_albumArt[1].clear();
            m_albumArt[0].swap(m_albumArt[1]);
            break;
        } default:
            break;
        }
    }
    QSharedPointer<QTemporaryFile> m_albumArt[2];
};

struct Player::Data {
    double volume = 1.0;
    MainWindow *mw = nullptr;
    PlayEngine *engine = nullptr;
    PlaylistModel *playlist = nullptr;
    QString playbackStatus, albumArt;
    QVariantMap metaData;
    Thread thread;
    struct {
        QTimer timer;
        bool flag = false;
    } started;
    auto toDBus(PlayEngine::State state) -> QString
    {
        switch (state) {
        case PlayEngine::Playing:
            return u"Playing"_q;
        case PlayEngine::Paused:
            return u"Paused"_q;
        default:
            return u"Stopped"_q;
        }
    }
    auto toDBus(const MetaData &md) const -> QVariantMap
    {
        QVariantMap map;
        const QDBusObjectPath path(dbusTrackId(md.mrl()));
        map[u"mpris:trackid"_q] = QVariant::fromValue(path);
        map[u"mpris:length"_q] = 1000LL*(qint64)md.duration();
        if (!albumArt.isEmpty())
            map[u"mpris:artUrl"_q] = _UrlFromLocalFile(albumArt).toString();
        map[u"xesam:url"_q] = md.mrl().toString();
        map[u"xesam:title"_q] = md.title().isEmpty()
                ? engine->media()->name() : md.title();
        if (!md.album().isEmpty())
            map[u"xesam:album"_q] = md.album();
        if (!md.artist().isEmpty())
            map[u"xesam:artist"_q] = QStringList(md.artist());
        if (!md.genre().isEmpty())
            map[u"xesam:genre"_q] = QStringList(md.genre());
        return map;
    }
};

Player::Player(QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , d(new Data)
{
    d->thread.p = this;
    d->thread.start();
    d->mw = cApp.mainWindow();
    d->engine = d->mw->engine();
    d->playlist = d->mw->playlist();

    d->playbackStatus = d->toDBus(d->engine->state());
    d->volume = d->engine->volume();
    d->metaData = d->toDBus(d->engine->metaData());
    connect(d->engine, &PlayEngine::metaDataChanged, this, &Player::updateMetaData);
    connect(d->engine, &PlayEngine::stateChanged, this,
            [this] (PlayEngine::State state) {
        d->playbackStatus = d->toDBus(d->engine->state());
        QVariantMap map;
        map[u"PlaybackStatus"_q] = d->playbackStatus;
        map[u"CanPause"_q] = map[u"CanPlay"_q] = state != PlayEngine::Error;
        sendPropertiesChanged(this, map);
    });
    connect(d->engine, &PlayEngine::speedChanged, this, [this] () {
        sendPropertiesChanged(this, "Rate", d->engine->speed());
    });
    auto checkNextPrevious = [this] () {
        QVariantMap map;
        map[u"CanGoNext"_q] = d->playlist->hasNext();
        map[u"CanGoPrevious"_q] = d->playlist->hasPrevious();
        sendPropertiesChanged(this, map);
    };
    connect(d->playlist, &PlaylistModel::loadedChanged,
            this, checkNextPrevious);
    connect(d->engine, &PlayEngine::seekableChanged, this,
            [this] (bool seekable) {
        sendPropertiesChanged(this, "CanSeek", seekable);
    });
    connect(d->engine, &PlayEngine::volumeChanged, this, [=] () {
        sendPropertiesChanged(this, "volume", d->engine->volume());
    });
    connect(d->engine, &PlayEngine::sought, this,
            [this] () { emit Seeked(time()); });

    // hack for nonsense interfere from MPRIS
    d->started.timer.setSingleShot(true);
    d->started.timer.setInterval(500);
    connect(d->engine, &PlayEngine::started, this, [=] () { d->started.timer.start(); });
    connect(d->engine, &PlayEngine::finished, this, [=] () {
        d->started.timer.stop(); d->started.flag = false;
        d->thread.saveAlbumArt(QImage());
    });
    connect(&d->started.timer, &QTimer::timeout, this, [=] () {
        d->started.flag = true;
        d->thread.saveAlbumArt(d->engine->snapshot());
    });
    d->thread.moveToThread(&d->thread);
}

Player::~Player() {
    d->thread.quit();
    if (d->thread.wait(30000))
        d->thread.terminate();
    d->thread.wait(30000);
    delete d;
}

auto Player::playbackStatus() const -> QString
{
    return d->playbackStatus;
}

auto Player::loopStatus() const -> QString
{
    return d->playlist->repeat() ? u"Playlist"_q : u"None"_q;
}

auto Player::setLoopStatus(const QString &status) -> void
{
    d->playlist->setRepeat(status != u"None"_q);
}

auto Player::rate() const -> double
{
    return d->engine->speed();
}

auto Player::setRate(double rate) -> void
{
    d->engine->setSpeed(rate);
}

auto Player::isShuffled() const -> bool
{
    return d->playlist->isShuffled();
}

auto Player::setShuffled(bool s) -> void
{
    d->playlist->setShuffled(s);
}

auto Player::metaData() const -> QVariantMap
{
    return d->metaData;
}

auto Player::volume() const -> double
{
    return d->volume;
}

auto Player::setVolume(double volume) -> void
{
    d->engine->setAudioVolume(qBound(0.0, volume, 1.0));
}

auto Player::time() const -> qint64
{
    const auto length = d->engine->time() - d->engine->begin();
    return qBound<qint64>(0LL, length, d->engine->duration()) * 1000LL;
}

auto Player::hasNext() const -> bool
{
    return d->playlist->hasNext();
}

auto Player::hasPrevious() const -> bool
{
    return d->playlist->hasPrevious();
}

auto Player::isPlayable() const -> bool
{
    return d->engine->state() != PlayEngine::Error;
}

auto Player::isPausable() const -> bool
{
    return d->engine->state() != PlayEngine::Error;
}

auto Player::isSeekable() const -> bool
{
    return d->engine->isSeekable();
}

auto Player::Next() -> void
{
    if (d->playlist->hasNext())
        d->playlist->playNext();
    else
        d->engine->stop();
}

auto Player::Previous() -> void
{
    if (d->playlist->hasPrevious())
        d->playlist->playPrevious();
    else
        d->engine->stop();
}

auto Player::Pause() -> void
{
    d->engine->pause();
}

auto Player::PlayPause() -> void
{
    d->mw->togglePlayPause();
}

auto Player::Stop() -> void
{
    d->engine->stop();
}

auto Player::Play() -> void
{
    d->mw->play();
}

auto Player::Seek(qint64 offset) -> void
{
    offset /= 1000;
    const auto duration = offset + d->engine->time() - d->engine->begin();
    if (d->engine->duration() > 0 && duration > d->engine->duration())
        Next();
    else
        d->engine->relativeSeek(offset);
}

auto Player::OpenUri(const QString &url) -> void
{
    d->mw->openFromFileManager(Mrl(QUrl(url)));
}

auto Player::SetPosition(const QDBusObjectPath &track, qint64 position) -> void
{
    if (!d->started.flag)
        return;
    position /= 1000;
    if (track.path() == dbusTrackId(d->engine->mrl())) {
        d->engine->seek(position + d->engine->begin());
    }
}

auto Player::updateMetaData() -> void
{
    d->metaData = d->toDBus(d->engine->metaData());
    sendPropertiesChanged(this, "Metadata", d->metaData);
}

auto Player::customEvent(QEvent *ev) -> void
{
    switch ((int)ev->type()) {
    case UpdateAlbumArt:
        d->albumArt = _MoveData<QString>(ev);
        updateMetaData();
        break;
    }
}

}
