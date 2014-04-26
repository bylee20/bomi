#include "letterboxitem.hpp"

LetterboxItem::LetterboxItem(QQuickItem *parent)
    : SimpleVertexItem(parent)
{
    setColor(Qt::black);
    vertices().resize(6*4);
}

bool LetterboxItem::set(const QRectF &outer, const QRectF &inner) {
    if (!(_Change(m_outer, outer) | _Change(m_inner, inner)))
        return false;
    m_screen = outer & inner;
    reserve(UpdateGeometry);
    polishAndUpdate();
    return true;
}

void LetterboxItem::updatePolish() {
    if (!isReserved(UpdateGeometry))
        return;
    auto it = vertices().begin();
    auto fill = [&it] (const QPointF &p1, const QPointF &p2) {
        it = Vertex::fillAsTriangles(it, p1, p2);
    };
    fill(m_outer.topLeft(), {m_outer.right(), m_inner.top()});
    fill({m_outer.left(), m_inner.bottom()}, m_outer.bottomRight());
    fill(m_outer.topLeft(), {m_inner.left(), m_outer.bottom()});
    fill({m_inner.right(), m_outer.top()}, m_outer.bottomRight());
}
