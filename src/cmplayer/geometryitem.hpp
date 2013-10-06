#ifndef GEOMETRYITEM_HPP
#define GEOMETRYITEM_HPP

#include <QQuickItem>

class GeometryItem : public QQuickItem {
	Q_OBJECT
public:
	GeometryItem(QQuickItem *parent = nullptr): QQuickItem(parent) {}
	void setGeometry(const QPointF &pos, const QSizeF &size) {setPosition(pos); setSize(size);}
	void setGeometry(const QRectF &rect) {setPosition(rect.topLeft()); setSize(rect.size());}
	QSizeF size() const { return {width(), height()}; }
	QRectF geometry() const { return {position(), size()}; }
	QRectF rect() const { return {0.0, 0.0, width(), height()}; }
	QPointF pos() const { return position(); }
	void setPos(const QPointF &pos) { setPosition(pos); }
};

#endif // GEOMETRYITEM_HPP
