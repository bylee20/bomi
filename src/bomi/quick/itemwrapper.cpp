#include "itemwrapper.hpp"
#include "appobject.hpp"
#include <QQmlEngine>

static QHash<const QMetaObject*, QMap<QByteArray, QMetaProperty>> propertyHash;

QtItem::QtItem(const QByteArray &source, QQuickItem *parent)
    : QtItem(parent)
{
    static QMap<QByteArray, QQmlComponent*> components;
    auto &component = components[source];
    if (!component) {
        auto engine = AppObject::qmlEngine();
        component = new QQmlComponent(engine, engine);
        component->setData(source, QUrl());
    }
    auto item = static_cast<QQuickItem*>(component->create());
    if (!component->errors().isEmpty())
        qDebug() << component->errorString();
    setItem(item);
    m_item->setParentItem(parent);
}

auto find(QQuickItem *object, const QByteArray &prop) -> QMetaProperty&
{
    auto &map = propertyHash[object->metaObject()];
    auto it = map.find(prop);
    if (it != map.end())
        return *it;
    QQmlProperty qml(object, QString::fromLatin1(prop));
    Q_ASSERT(qml.isValid());
    return *map.insert(prop, qml.property());
}

auto QtItem::set(const QByteArray &prop, const QVariant &var) -> void
{
    find(m_item, prop).write(m_item, var);
}

auto QtItem::get(const QByteArray &prop) const -> QVariant
{
    return find(m_item, prop).read(m_item);
}

auto QtItem::notify(const QByteArray &prop, QObject *dest, const char *slot) -> void
{
    auto &p = find(m_item, prop);
    auto mo = dest->metaObject();
    auto con = connect(m_item, p.notifySignal(), dest, mo->method(mo->indexOfSlot(slot)));
    Q_ASSERT(con);
}

auto QtItem::setItem(QQuickItem *item) -> void
{
    Q_ASSERT(!m_item && item);
    m_item = item;
    for (int i=0; i<6; ++i) {
        const auto name = anchorName(static_cast<AnchorLine>(i));
        const QString prefix = "anchors."_a % name;
        m_anchors[i] = QQmlProperty(m_item, prefix);
        if (i < 4)
            m_margins[i] = QQmlProperty(m_item, prefix % "Margin"_a);
        else
            m_margins[i] = QQmlProperty(m_item, prefix % "Offset"_a);
    }
}

auto QtItem::wrap(QQuickItem *item) -> QtItem*
{
    auto qt = new QtItem;
    item->setParent(qt);
    qt->setItem(item);
    return qt;
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
