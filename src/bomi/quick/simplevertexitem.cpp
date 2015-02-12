#include "simplevertexitem.hpp"
#include <QSGFlatColorMaterial>

SimpleVertexItem::SimpleVertexItem(QQuickItem *parent)
    : VertexDrawItem<OGL::PositionVertex>(parent)
{
}

SimpleVertexItem::~SimpleVertexItem() {

}

auto SimpleVertexItem::createMaterial() const -> QSGMaterial*
{
    return new QSGFlatColorMaterial;
}

auto SimpleVertexItem::updateMaterial(QSGMaterial *material) -> QSGMaterial*
{
    auto m = static_cast<QSGFlatColorMaterial*>(material);
    m->setColor(m_color);
    m->setFlag(QSGMaterial::Blending, m_color.alpha() < 255);
    return m;
}

auto SimpleVertexItem::setColor(const QColor &color) -> void
{
    if (_Change(m_color, color)) {
        reserve(UpdateMaterial);
        update();
        emit colorChanged();
    }
}
