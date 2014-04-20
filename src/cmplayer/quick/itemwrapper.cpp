#include "itemwrapper.hpp"
#include "globalqmlobject.hpp"

QtItem::QtItem(const QByteArray &source, QQuickItem *parent)
	: QObject(parent)
{
	static QMap<QByteArray, QQmlComponent*> components;
	auto &component = components[source];
	if (!component) {
		auto engine = UtilObject::qmlEngine();
		component = new QQmlComponent(engine, engine);
		component->setData(source, QUrl());
	}
	m_item = static_cast<QQuickItem*>(component->create());
	m_item->setParentItem(parent);

	m_anchors.resize(6);
	m_margins.resize(6);
	for (int i=0; i<6; ++i) {
		const auto name = anchorName(static_cast<AnchorLine>(i));
		const QString prefix = _L("anchors.") % name;
		m_anchors[i] = QQmlProperty(m_item, prefix);
		if (i < 4)
			m_margins[i] = QQmlProperty(m_item, prefix % _L("Margin"));
		else
			m_margins[i] = QQmlProperty(m_item, prefix % _L("Offset"));
	}
}

TextItem::TextItem(QQuickItem *parent)
	: QtItem("import QtQuick 2.0\nText {}\n", parent)
{
	m_text = qmlProperty("text");
	m_vAlign = qmlProperty("verticalAlignment");
	m_hAlign = qmlProperty("horizontalAlignment");
	m_color = qmlProperty("color");
}

RectangleItem::RectangleItem(QQuickItem *parent)
	: RectangleItem("import QtQuick 2.0\nRectangle {}\n", parent) { }

RectangleItem::RectangleItem(const QByteArray &source, QQuickItem *parent)
	: QtItem(source, parent)
{
	m_color = qmlProperty("color");
	m_radius = qmlProperty("radius");
}
