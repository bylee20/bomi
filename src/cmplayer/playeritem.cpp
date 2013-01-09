#include "playeritem.hpp"
#include "playengine.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "audiocontroller.hpp"
#include "subtitlerendereritem.hpp"
#include "rootmenu.hpp"

PlayerItem::PlayerItem(QQuickItem *parent)
: QQuickItem(parent) {
	m_video = new VideoRendererItem(this);
	m_subtitle = m_video->subtitle();
	m_info = new PlayInfoItem(this);
	m_audio = new AudioController(this);
}

void PlayerItem::plug(PlayEngine *engine) {
	if (m_engine && m_engine != engine)
		unplug();

	auto &menu = RootMenu::get();
	connect(m_audio, &AudioController::mutedChanged, menu("audio")["mute"], &QAction::setChecked);
//	CONNECT(&d->audio, volumeNormalizedChanged(bool), audio["volnorm"], setChecked(bool));

	m_engine = engine;
	::plug(m_engine, m_video);
	::plug(m_engine, m_audio);
	m_info->set(m_engine, m_video, m_audio);
}

PlayEngine *PlayerItem::unplug() {
	if (m_engine) {
		::unplug(m_engine, m_video);
		::unplug(m_engine, m_audio);
	}
	return m_engine;
}

void PlayerItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
	m_video->setPosition(QPointF(0, 0));
	m_video->setSize(QSizeF(width(), height()));
}

void PlayerItem::registerItems() {
	qmlRegisterType<VideoRendererItem>("CMPlayer", 1, 0, "VideoRenderer");
	qmlRegisterType<PlayInfoItem>("CMPlayer", 1, 0, "PlayInfo");
	qmlRegisterType<AvInfoObject>();
	qmlRegisterType<AvIoFormat>();
	qmlRegisterType<MediaInfoObject>();
	qmlRegisterType<SubtitleRendererItem>("CMPlayer", 1, 0, "SubtitleRenderer");
	qmlRegisterType<PlayerItem>("CMPlayer", 1, 0, "Player");
}

bool PlayerItem::execute(const QString &key) {
	auto action = RootMenu::get().action(key);
	if (!action)
		return false;
	if (action->menu())
		action->menu()->exec(QCursor::pos());
	else
		action->trigger();
	return true;
}

void PlayerItem::seek(int time) {
	if (m_engine) {
		m_engine->seek(time);
	}
}

void PlayerItem::setVolume(int volume) {
	m_audio->setVolume(volume);
}
