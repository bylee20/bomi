#include "mpris.hpp"
#include "playengine.hpp"
#include "playlistmodel.hpp"
#include "globalqmlobject.hpp"
#include "mainwindow.hpp"
#include "app.hpp"
#include "mediamisc.hpp"

namespace mpris {

RootObject::RootObject(QObject *parent)
: QObject(parent) {
	m_name = _L("org.mpris.MediaPlayer2.CMPlayer");
	auto ok = QDBusConnection::sessionBus().registerService(m_name);
	if (!ok)
		ok = QDBusConnection::sessionBus().registerService(m_name = m_name % ".instance" % QString::number(cApp.applicationPid()));
	if (!ok)
		m_name.clear();
	else {
		m_mp2 = new MediaPlayer2(this);
		m_player = new Player(this);
		QDBusConnection::sessionBus().registerObject("/org/mpris/MediaPlayer2", this, QDBusConnection::ExportAdaptors);
	}
}

RootObject::~RootObject() {
	if (m_name.isEmpty())
		return;
	QDBusConnection::sessionBus().unregisterObject("/org/mpris/MediaPlayer2");
	QDBusConnection::sessionBus().unregisterService(m_name);
}


static QString dbusTrackId(const Mrl &mrl) {
	return _L("/net/xylosper/CMPlayer/track_") + QCryptographicHash::hash(mrl.toString().toLocal8Bit(), QCryptographicHash::Md5).toHex();
}

static void sendPropertiesChanged(const QDBusAbstractAdaptor *adaptor, const QVariantMap &properties) {
	const auto iface = _L(adaptor->metaObject()->classInfo(0).value());
	auto sig = QDBusMessage::createSignal("/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged" );
	sig << iface << properties << QStringList();
	QDBusConnection::sessionBus().send(sig);
}

static void sendPropertiesChanged(const QDBusAbstractAdaptor *adaptor, const char *property, const QVariant &value) {
	QVariantMap properties; properties.insert(property, value); sendPropertiesChanged(adaptor, properties);
}

struct MediaPlayer2::Data {
	MainWindow *mw = nullptr;
};

MediaPlayer2::MediaPlayer2(QObject *parent)
: QDBusAbstractAdaptor(parent), d(new Data) {
	d->mw = cApp.mainWindow();
	Q_ASSERT(d->mw);
	auto util = &UtilObject::get();
	connect(util, &UtilObject::fullScreenChanged, [this] (bool fs) {
		sendPropertiesChanged(this, "Fullscreen", fs);
	});
}

MediaPlayer2::~MediaPlayer2() {
	delete d;
}

bool MediaPlayer2::isFullScreen() const {
	return d->mw->isFullScreen();
}

void MediaPlayer2::setFullScreen(bool fs) {
	d->mw->setFullScreen(fs);
}

void MediaPlayer2::Raise() {
	d->mw->wake();
}

void MediaPlayer2::Quit() {
	d->mw->exit();
}

QStringList MediaPlayer2::supportedUriSchemes() const {
	return QStringList() << "file" << "http" << "https" << "ftp" << "mms" << "rtmp" << "rtsp" << "sftp";
}

QStringList MediaPlayer2::supportedMimeTypes() const {
	return QStringList() << "application/ogg" << "application/x-ogg" << "application/sdp" << "application/smil"
		<< "application/x-smil" << "application/streamingmedia" << "application/x-streamingmedia" << "application/vnd.rn-realmedia"
		<< "application/vnd.rn-realmedia-vbr" << "audio/aac" << "audio/x-aac" << "audio/m4a" << "audio/x-m4a" << "audio/mp1"
		<< "audio/x-mp1" << "audio/mp2" << "audio/x-mp2" << "audio/mp3" << "audio/x-mp3" << "audio/mpeg" << "audio/x-mpeg"
		<< "audio/mpegurl" << "audio/x-mpegurl" << "audio/mpg" << "audio/x-mpg" << "audio/rn-mpeg" << "audio/scpls" << "audio/x-scpls"
		<< "audio/vnd.rn-realaudio" << "audio/wav" << "audio/x-pn-windows-pcm" << "audio/x-realaudio" << "audio/x-pn-realaudio"
		<< "audio/x-ms-wma" << "audio/x-pls" << "audio/x-wav" << "video/mpeg" << "video/x-mpeg" << "video/x-mpeg2" << "video/mp4"
		<< "video/msvideo" << "video/x-msvideo" << "video/quicktime" << "video/vnd.rn-realvideo" << "video/x-ms-afs"
		<< "video/x-ms-asf" << "video/x-ms-wmv" << "video/x-ms-wmx" << "video/x-ms-wvxvideo" << "video/x-avi" << "video/x-fli"
		<< "video/x-flv" << "video/x-theora" << "video/x-matroska" << "video/mp2t";
}

/********************************************************************************/

struct Player::Data {
	double volume = 1.0;
	MainWindow *mw = nullptr;
	PlayEngine *engine = nullptr;
	PlaylistModel *playlist = nullptr;
	QString playbackStatus;
	QVariantMap metaData;
	QString toDBus(PlayEngine::State state) {
		switch (state) {
		case PlayEngine::Playing:
			return _L("Playing");
		case PlayEngine::Paused:
		case PlayEngine::Buffering:
			return _L("Paused");
		default:
			return _L("Stopped");
		}
	}
	QVariantMap toDBus(const MetaData &md) const {
		QVariantMap map;
		map["mpris:trackid"] = QVariant::fromValue(QDBusObjectPath(dbusTrackId(md.mrl())));
		map["mpris:length"] = 1000LL*(qint64)md.duration();
		map["xesam:url"] = md.mrl().toString();
		map["xesam:title"] = md.title().isEmpty() ? engine->mediaInfo()->name() : md.title();
		if (!md.album().isEmpty())
			map["xesam:album"] = md.album();
		if (!md.artist().isEmpty())
			map["xesam:artist"] = QStringList(md.artist());
		if (!md.genre().isEmpty())
			map["xesam:genre"] = QStringList(md.genre());
		return map;
	}
};

Player::Player(QObject *parent): QDBusAbstractAdaptor(parent), d(new Data) {
	d->mw = cApp.mainWindow();
	d->engine = d->mw->engine();
	d->playlist = d->mw->playlist();

	d->playbackStatus = d->toDBus(d->engine->state());
	d->volume = d->engine->volume();
	d->metaData = d->toDBus(d->engine->metaData());
	connect(d->engine, &PlayEngine::metaDataChanged, this, [this] () {
		d->metaData = d->toDBus(d->engine->metaData());
		qDebug() << d->metaData;
		sendPropertiesChanged(this, "Metadata", d->metaData);
	});
	connect(d->engine, &PlayEngine::stateChanged, this, [this] (PlayEngine::State state) {
		d->playbackStatus = d->toDBus(d->engine->state());
		QVariantMap map;
		map["PlaybackStatus"] = d->playbackStatus;
		map["CanPause"] = map["CanPlay"] = state != PlayEngine::Error;
		sendPropertiesChanged(this, map);
	});
	connect(d->engine, &PlayEngine::speedChanged, this, [this] (double speed) {
		sendPropertiesChanged(this, "Rate", speed);
	});
	auto checkNextPrevious = [this] () {
		QVariantMap map;
		map["CanGoNext"] = d->playlist->hasNext();
		map["CanGoPrevious"] = d->playlist->hasPrevious();
		sendPropertiesChanged(this, map);
	};
	connect(d->playlist, &PlaylistModel::loadedChanged, this, checkNextPrevious);
	connect(d->engine, &PlayEngine::seekableChanged, this, [this] (bool seekable) {
		sendPropertiesChanged(this, "CanSeek", seekable);
	});
	connect(d->engine, &PlayEngine::sought, this, [this] () { qDebug() << "seekd"; emit Seeked(time()); });
}

Player::~Player() {
	delete d;
}

QString Player::playbackStatus() const {
	return d->playbackStatus;
}

QString Player::loopStatus() const {
	return _L("None");
}

void Player::setLoopStatus(const QString &/*status*/) {

}

double Player::rate() const {
	return d->engine->speed();
}

void Player::setRate(double rate) {
	d->engine->setSpeed(rate);
}

bool Player::isShuffled() const {
	return false;
}

void Player::setShuffled(bool /*s*/) {

}

QVariantMap Player::metaData() const {
	return d->metaData;
}

double Player::volume() const {
	return d->volume;
}

void Player::setVolume(double volume) {
	if (_Change(d->volume, qBound(0.0, volume, 1.0))) {
		d->engine->setVolume(d->volume*100 + 0.5);
		sendPropertiesChanged(this, "Volume", d->volume);
	}
}

qint64 Player::time() const {
	return qBound<qint64>(0LL, d->engine->time()-d->engine->begin(), d->engine->duration())*1000LL;
}

bool Player::hasNext() const {
	return d->playlist->hasNext();
}

bool Player::hasPrevious() const {
	return d->playlist->hasPrevious();
}

bool Player::isPlayable() const {
	return d->engine->state() != PlayEngine::Error;
}

bool Player::isPausable() const {
	return d->engine->state() != PlayEngine::Error;
}

bool Player::isSeekable() const {
	return d->engine->isSeekable();
}

void Player::Next() {
	if (d->playlist->hasNext())
		d->playlist->playNext();
	else
		d->engine->stop();
}

void Player::Previous() {
	if (d->playlist->hasPrevious())
		d->playlist->playPrevious();
	else
		d->engine->stop();
}

void Player::Pause() {
	d->engine->pause();
}

void Player::PlayPause() {
	d->mw->togglePlayPause();
}

void Player::Stop() {
	d->engine->stop();
}

void Player::Play() {
	d->mw->play();
}

void Player::Seek(qint64 offset) {
	offset /= 1000;
	if (d->engine->duration() > 0 && (offset + d->engine->time() - d->engine->begin() > d->engine->duration()))
		Next();
	else
		d->engine->relativeSeek(offset);
}

void Player::OpenUri(const QString &url) {
	d->mw->openFromFileManager(Mrl(QUrl(url)));
}

void Player::SetPosition(const QDBusObjectPath &track, qint64 position) {
	position /= 1000;
	if (track.path() == dbusTrackId(d->engine->mrl()))
		d->engine->seek(position + d->engine->begin());
}



}
