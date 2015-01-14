#include "geometryitem.hpp"

auto GeometryItem::geometryChanged(const QRectF &n, const QRectF &o) -> void
{
    QQuickItem::geometryChanged(n, o);
    if (_Change(m_size, n.size()))
        emit sizeChanged(m_size);
}
