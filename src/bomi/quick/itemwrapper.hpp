#ifndef ITEMWRAPPER_HPP
#define ITEMWRAPPER_HPP

#include <QQuickItem>
#include <QQmlProperty>

class QtItem : public QObject {
public:
    enum AnchorLine {
        Top, Left, Bottom, Right,
        VerticalCenter, HorizontalCenter
    };
    explicit QtItem(const QByteArray &source, QQuickItem *parent = nullptr);
    ~QtItem() { delete m_item; }
    QQuickItem *item() const { return m_item; }
    auto clearAnchor(AnchorLine line) -> void;
    auto clearAnchors() -> void;
    auto anchor(AnchorLine line, QQuickItem *target, AnchorLine to) -> void;
    auto anchor(AnchorLine line, QQuickItem *target) -> void;
    auto anchorCenterIn(QQuickItem *target) -> void;
    auto anchorFill(QQuickItem *target) -> void;
    auto setMargin(AnchorLine line, qreal margin) -> void;
    auto setMargins(qreal margin) -> void;
    auto setOffset(AnchorLine line, qreal offset) -> void;
    auto setOffsets(qreal offset) -> void;
    auto setVisible(bool visible) -> void { m_item->setVisible(visible); }
    auto isVisible() const -> bool { return m_item->isVisible(); }
    auto setWidth(qreal w) -> void { m_item->setWidth(w); }
    auto setHeight(qreal h) -> void { m_item->setHeight(h); }
    auto width() const -> qreal { return m_item->width(); }
    auto height() const -> qreal { return m_item->height(); }
    auto implicitWidth() const -> qreal { return m_item->implicitWidth(); }
    auto implicitHeight() const -> qreal { return m_item->implicitHeight(); }
    auto size() const -> QSizeF { return QSizeF(width(), height()); }
    auto resize(const QSizeF &s) -> void { resize(s.width(), s.height()); }
    auto resize(qreal w, qreal h) -> void { setWidth(w); setHeight(h); }
    auto pos() const -> QPointF { return m_item->position(); }
    auto move(const QPointF &pos) -> void { move(pos.x(), pos.y()); }
    auto move(qreal x, qreal y) -> void { setX(x); setY(y); }
    auto setX(qreal x) -> void { m_item->setX(x); }
    auto setY(qreal y) -> void { m_item->setY(y); }
    auto setZ(qreal z) -> void { m_item->setZ(z); }
    auto x() const -> qreal { return m_item->x(); }
    auto y() const -> qreal { return m_item->y(); }
    auto z() const -> qreal { return m_item->z(); }
    auto set(const QByteArray &prop, const QVariant &var) -> void;
    auto get(const QByteArray &property) const -> QVariant;
    auto notify(const QByteArray &property, QObject *dest, const char *slot) -> void;
    static auto wrap(QQuickItem *item) -> QtItem*;
protected:
    auto qmlProperty(const char *name) const -> QQmlProperty
    { return QQmlProperty(m_item, _L(name)); }
    auto qmlProperty(const QByteArray &name) const -> QQmlProperty
    { return QQmlProperty(m_item, QString::fromLatin1(name)); }
private:
    QtItem(QObject *parent = nullptr)
        : QObject(parent) { m_anchors.resize(6); m_margins.resize(6); }
    auto setItem(QQuickItem *item) -> void;
    static auto anchorName(AnchorLine line) -> QString;
    QQuickItem *m_item = nullptr;
    QVector<QQmlProperty> m_anchors, m_margins;
};

inline auto QtItem::clearAnchor(AnchorLine line) -> void
{ m_anchors[line].write(QVariant()); }

inline auto QtItem::clearAnchors() -> void
{ for (auto &anchor : m_anchors) anchor.write(QVariant()); }

inline auto QtItem::anchor(AnchorLine line,
                           QQuickItem *target, AnchorLine to) -> void
{ m_anchors[line].write(QQmlProperty(target, anchorName(to)).read()); }


inline auto QtItem::anchor(AnchorLine line, QQuickItem *target) -> void
{ anchor(line, target, line); }

inline auto QtItem::anchorCenterIn(QQuickItem *target) -> void
{
    anchor(VerticalCenter, target, VerticalCenter);
    anchor(HorizontalCenter, target, HorizontalCenter);
}

inline auto QtItem::anchorFill(QQuickItem *target) -> void
{
    anchor(Top, target, Top);
    anchor(Left, target, Left);
    anchor(Bottom, target, Bottom);
    anchor(Right, target, Right);
}

inline auto QtItem::setMargin(AnchorLine line, qreal margin) -> void
{ m_margins[line].write(margin); }

inline auto QtItem::setMargins(qreal margin) -> void
{
    setMargin(Top, margin);
    setMargin(Left, margin);
    setMargin(Bottom, margin);
    setMargin(Right, margin);
}

inline auto QtItem::setOffset(AnchorLine line, qreal offset) -> void
{ m_margins[line].write(offset); }

inline auto QtItem::setOffsets(qreal offset) -> void
{
    setOffset(VerticalCenter, offset);
    setOffset(HorizontalCenter, offset);
}

inline auto QtItem::anchorName(AnchorLine line) -> QString
{
    static const QVector<QString> names = [] () {
        QVector<QString> names(6);
        names[Top] = u"top"_q;
        names[Left] = u"left"_q;
        names[Bottom] = u"bottom"_q;
        names[Right] = u"right"_q;
        names[VerticalCenter] = u"verticalCenter"_q;
        names[HorizontalCenter] = u"horizontalCenter"_q;
        return names;
    }();
    return names[line];
}

/******************************************************************************/

class TextItem : public QtItem {
public:
    explicit TextItem(QQuickItem *parent = nullptr);
    auto text() const -> QString { return m_text.read().toString(); }
    auto setText(const QString &text) -> void { m_text.write(text); }
    auto alignment() const -> Qt::Alignment;
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setColor(const QColor &color) -> void;
    auto color() const -> QColor { return m_color.read().value<QColor>(); }
private:
    QQmlProperty m_text, m_vAlign, m_hAlign, m_color;
};

inline auto TextItem::alignment() const -> Qt::Alignment
{
    const auto v = m_vAlign.read().toInt();
    const auto h = m_hAlign.read().toInt();
    return static_cast<Qt::Alignment>(v | h);
}

inline auto TextItem::setAlignment(Qt::Alignment alignment) -> void
{
    const int v = alignment & Qt::AlignVertical_Mask;
    const int h = alignment & Qt::AlignHorizontal_Mask;
    m_vAlign.write(v);
    m_hAlign.write(h);
}

inline auto TextItem::setColor(const QColor &color) -> void
{ m_color.write(QVariant::fromValue(color)); }

/******************************************************************************/

class RectangleItem : public QtItem {
public:
    RectangleItem(QQuickItem *parent = nullptr);
    RectangleItem(const QByteArray &source, QQuickItem *parent = nullptr);
    auto radius() const -> qreal { return m_radius.read().value<qreal>(); }
    auto setRadius(qreal radius) -> void { m_radius.write(radius); }
    auto color() const -> QColor { return m_color.read().value<QColor>(); }
    auto setColor(const QColor &color) -> void;
private:
    QQmlProperty m_color, m_radius;
};

inline auto RectangleItem::setColor(const QColor &color) -> void
{ m_color.write(QVariant::fromValue(color)); }

#endif // ITEMWRAPPER_HPP
