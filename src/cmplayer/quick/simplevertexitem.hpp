#ifndef SIMPLEVERTEXITEM_HPP
#define SIMPLEVERTEXITEM_HPP

#include "opengldrawitem.hpp"
#include "opengl/openglvertex.hpp"

class SimpleVertexItem : public VertexDrawItem<OGL::PositionVertex> {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    SimpleVertexItem(QQuickItem *parent = nullptr);
    ~SimpleVertexItem();
    QColor color() const { return m_color; }
    void setColor(const QColor &color);
signals:
    void colorChanged();
private:
    QSGMaterial *createMaterial() const override final;
    QSGMaterial *updateMaterial(QSGMaterial *material) override final;
    QColor m_color;
};

#endif // SIMPLEVERTEXITEM_HPP
