#include "playeritem.hpp"
#include "playengine.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "subtitlerendereritem.hpp"
#include "rootmenu.hpp"

PlayerItem::PlayerItem(QQuickItem *parent)
: QQuickItem(parent) {
	m_info = new PlayInfoItem(this);
}

void PlayerItem::create() {
	if (!m_renderer) {
		m_renderer = new VideoRendererItem(this);
		m_subtitle = m_renderer->subtitle();
		m_renderer->setZ(-1);
	}
}

void PlayerItem::plugTo(PlayEngine *engine) {
	if (m_engine != engine) {
		m_engine = engine;
		m_engine->setVideoRenderer(m_renderer);
		m_info->set(m_engine);
	}
}

void PlayerItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
	if (m_renderer) {
		m_renderer->setPosition(QPointF(0, 0));
		m_renderer->setSize(QSizeF(width(), height()));
	}
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
	if (m_engine) {
		if (action->menu())
			action->menu()->exec(QCursor::pos());
		else
			action->trigger();
	}
	return true;
}

void PlayerItem::seek(int time) {
	if (m_engine) {
		m_engine->seek(time);
	}
}

void PlayerItem::setVolume(int volume) {
	if (m_engine)
		m_engine->setVolume(volume);
}
