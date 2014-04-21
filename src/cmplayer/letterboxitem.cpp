#include "letterboxitem.hpp"

LetterboxItem::LetterboxItem(QQuickItem *parent)
: QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

bool LetterboxItem::set(const QRectF &outer, const QRectF &inner) {
    if (m_outer != outer || m_inner != inner) {
        m_outer = outer;
        m_inner = inner;
        m_screen = outer & inner;
        m_rectChanged = true;
        update();
        return true;
    } else
        return false;
}

QSGNode *LetterboxItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
    Q_UNUSED(data);
    QSGGeometryNode *node = static_cast<QSGGeometryNode*>(old);
    QSGGeometry *geometry = 0;

    if (!node) {
        node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 3*2*4);
        geometry->setDrawingMode(GL_TRIANGLES);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);
        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(Qt::black);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
        m_rectChanged = true;
    } else {
        geometry = node->geometry();
    }

    if (m_rectChanged) {

        auto vtx = geometry->vertexDataAsPoint2D();

        auto make = [this, &vtx] (const QPointF &p1, const QPointF &p2) {
            vtx++->set(p1.x(), p1.y());
            vtx++->set(p2.x(), p1.y());
            vtx++->set(p1.x(), p2.y());

            vtx++->set(p1.x(), p2.y());
            vtx++->set(p2.x(), p2.y());
            vtx++->set(p2.x(), p1.y());
        };

        make(m_outer.topLeft(), {m_outer.right(), m_inner.top()});
        make({m_outer.left(), m_inner.bottom()}, m_outer.bottomRight());
        make(m_outer.topLeft(), {m_inner.left(), m_outer.bottom()});
        make({m_inner.right(), m_outer.top()}, m_outer.bottomRight());

        m_rectChanged = false;
        node->markDirty(QSGNode::DirtyGeometry);
    }
    return node;
}

