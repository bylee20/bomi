#ifndef TEXTITEMWRAPPER_HPP
#define TEXTITEMWRAPPER_HPP

#include "stdafx.hpp"

class QtItem : public QObject {
	Q_OBJECT
public:
	enum AnchorLine { Top, Left, Bottom, Right, VerticalCenter, HorizontalCenter };
	explicit QtItem(const QByteArray &source, QQuickItem *parent = nullptr);
	~QtItem() { delete m_item; }
	QQuickItem *item() const { return m_item; }
	void clearAnchor(AnchorLine line) { m_anchors[line].write(QVariant()); }
	void clearAnchors() { for (auto &anchor : m_anchors) anchor.write(QVariant()); }
	void anchor(AnchorLine line, QQuickItem *target, AnchorLine to) {
		m_anchors[line].write(QQmlProperty(target, anchorName(to)).read());
	}
	void anchorCenterIn(QQuickItem *target) {
		anchor(VerticalCenter, target, VerticalCenter);
		anchor(HorizontalCenter, target, HorizontalCenter);
	}
	void anchorFill(QQuickItem *target) {
		anchor(Top, target, Top);
		anchor(Left, target, Left);
		anchor(Bottom, target, Bottom);
		anchor(Right, target, Right);
	}
	void setMargin(AnchorLine line, qreal margin) {
		m_margins[line].write(margin);
	}
	void setMargins(qreal margin) {
		setMargin(Top, margin);
		setMargin(Left, margin);
		setMargin(Bottom, margin);
		setMargin(Right, margin);
	}
	void setOffset(AnchorLine line, qreal offset) {
		m_margins[line].write(offset);
	}
	void setOffsets(qreal offset) {
		setOffset(VerticalCenter, offset);
		setOffset(HorizontalCenter, offset);
	}
	void setVisible(bool visible) { m_item->setVisible(visible); }
	bool isVisible() const { return m_item->isVisible(); }
	void setWidth(qreal w) { m_item->setWidth(w); }
	void setHeight(qreal h) { m_item->setHeight(h); }
	qreal width() const { return m_item->width(); }
	qreal height() const { return m_item->height(); }
	QSizeF size() const { return QSizeF(width(), height()); }
	void resize(const QSizeF &size) { resize(size.width(), size.height()); }
	void resize(qreal w, qreal h) { setWidth(w); setHeight(h); }
	QPointF pos() const { return m_item->position(); }
	void move(const QPointF &pos) { move(pos.x(), pos.y()); }
	void move(qreal x, qreal y) { setX(x); setY(y); }
	void setX(qreal x) { m_item->setX(x); }
	void setY(qreal y) { m_item->setY(y); }
	void setZ(qreal z) { m_item->setZ(z); }
	qreal x() const { return m_item->x(); }
	qreal y() const { return m_item->y(); }
	qreal z() const { return m_item->z(); }
protected:
	QQmlProperty qmlProperty(const char *name) const {
		return QQmlProperty(m_item, _L(name));
	}
private:
	static const char *anchorName(AnchorLine line) {
		static const QVector<const char *> names = [] () {
			QVector<const char*> names(6);
			names[Top] = "top";
			names[Left] = "left";
			names[Bottom] = "bottom";
			names[Right] = "right";
			names[VerticalCenter] = "verticalCenter";
			names[HorizontalCenter] = "horizontalCenter";
			return names;
		}();
		return names[line];
	}
	QQuickItem *m_item = nullptr;
	QVector<QQmlProperty> m_anchors, m_margins;
};

class TextItem : public QtItem {
	Q_OBJECT
public:
	explicit TextItem(QQuickItem *parent = nullptr);
	QString text() const { return m_text.read().toString(); }
	void setText(const QString &text) { m_text.write(text); }
	Qt::Alignment alignment() const {
		const auto v = m_vAlign.read().toInt();
		const auto h = m_hAlign.read().toInt();
		return static_cast<Qt::Alignment>(v | h);
	}
	void setAlignment(Qt::Alignment alignment) {
		const int v = alignment & Qt::AlignVertical_Mask;
		const int h = alignment & Qt::AlignHorizontal_Mask;
		m_vAlign.write(v);
		m_hAlign.write(h);
	}
	void setColor(const QColor &color) { m_color.write(QVariant::fromValue(color)); }
	QColor color() const { return m_color.read().value<QColor>(); }
private:
	QQmlProperty m_text, m_vAlign, m_hAlign, m_color;
};

class RectangleItem : public QtItem {
	Q_OBJECT
public:
	RectangleItem(QQuickItem *parent = nullptr);
	RectangleItem(const QByteArray &source, QQuickItem *parent = nullptr);
	qreal radius() const { return m_radius.read().value<qreal>(); }
	void setRadius(qreal radius) { m_radius.write(radius); }
	QColor color() const { return m_color.read().value<QColor>(); }
	void setColor(const QColor &color) { m_color.write(QVariant::fromValue(color)); }
private:
	QQmlProperty m_color, m_radius;
};

#endif // TEXTITEMWRAPPER_HPP
