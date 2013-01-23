#include "playeritem.hpp"
#include "playengine.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "rootmenu.hpp"
#include "globalqmlobject.hpp"

static QObject *utilProvider(QQmlEngine *, QJSEngine *) {return new GlobalQmlObject;}

PlayerItem::PlayerItem(QQuickItem *parent)
: QQuickItem(parent) {
	m_info = new PlayInfoItem(this);
}

void PlayerItem::unplug() {
	Skin::unplug();
	if (m_renderer)
		m_renderer->setParentItem(nullptr);
}

void PlayerItem::plugTo(PlayEngine *engine) {
	if (m_engine != engine) {
		m_engine = engine;
		m_renderer = engine->videoRenderer();
		m_renderer->setParentItem(this);
		m_renderer->setGeometry(QPointF(0, 0), QSizeF(width(), height()));
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
	qmlRegisterType<AvInfoObject>();
	qmlRegisterType<AvIoFormat>();
	qmlRegisterType<MediaInfoObject>();
	qmlRegisterType<PlayInfoItem>("CMPlayer", 1, 0, "PlayInfo");
	qmlRegisterType<PlayerItem>("CMPlayer", 1, 0, "Player");
	qmlRegisterSingletonType<GlobalQmlObject>("CMPlayer", 1, 0, "Util", utilProvider);
}

bool PlayerItem::execute(const QString &key) {
	auto action = cMenu.action(key);
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
