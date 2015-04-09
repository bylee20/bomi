#include "geometryitem.hpp"

auto GeometryItem::geometryChanged(const QRectF &n, const QRectF &o) -> void
{
    QQuickItem::geometryChanged(n, o);
    m_size = n.size();
}
