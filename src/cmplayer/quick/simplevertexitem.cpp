#include "simplevertexitem.hpp"

SimpleVertexItem::SimpleVertexItem(QQuickItem *parent)
    : VertexDrawItem<OGL::PositionVertex>(parent)
{
}

SimpleVertexItem::~SimpleVertexItem() {

}

QSGMaterial *SimpleVertexItem::createMaterial() const {
    return new QSGFlatColorMaterial;
}

QSGMaterial *SimpleVertexItem::updateMaterial(QSGMaterial *material) {
    auto m = static_cast<QSGFlatColorMaterial*>(material);
    m->setColor(m_color);
    m->setFlag(QSGMaterial::Blending, m_color.alpha() < 255);
    return m;
}

void SimpleVertexItem::setColor(const QColor &color) {
    if (_Change(m_color, color)) {
        reserve(UpdateMaterial);
        update();
        emit colorChanged();
    }
}
