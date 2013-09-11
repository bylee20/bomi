#ifndef LETTERBOXITEM_HPP
#define LETTERBOXITEM_HPP

#include "stdafx.hpp"

class LetterboxItem : public QQuickItem {
public:
	LetterboxItem(QQuickItem *parent = 0);
	QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
	bool set(const QRectF &outer, const QRectF &inner);
	const QRectF &screen() {return m_screen;}
private:
	QRectF m_outer, m_inner, m_screen;
	bool m_rectChanged;
};

#endif // LETTERBOXITEM_HPP
