#ifndef TRIANGLEITEM_HPP
#define TRIANGLEITEM_HPP

#include "opengldrawitem.hpp"
#include "opengl/openglvertex.hpp"

class TriangleItem : public ShaderRenderItem<OGL::ColorVertex> {
    Q_OBJECT
    Q_PROPERTY(QPointF p1 READ p1 WRITE setP1 NOTIFY p1Changed)
    Q_PROPERTY(QPointF p2 READ p2 WRITE setP2 NOTIFY p2Changed)
    Q_PROPERTY(QPointF p3 READ p3 WRITE setP3 NOTIFY p3Changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    TriangleItem(QQuickItem *parent = nullptr);
    ~TriangleItem();
    auto p1() const -> QPointF;
    auto p2() const -> QPointF;
    auto p3() const -> QPointF;
    auto setP1(const QPointF &p) -> void;
    auto setP2(const QPointF &p) -> void;
    auto setP3(const QPointF &p) -> void;
    auto color() const -> QColor;
    auto setColor(const QColor &color) -> void;
signals:
    void p1Changed(const QPointF &p1);
    void p2Changed(const QPointF &p2);
    void p3Changed(const QPointF &p3);
    void colorChanged(const QColor &color);
private:
    auto geometryChanged(const QRectF &new_, const QRectF &old) -> void final;
    auto updatePolish() -> void final;
    auto vertexCount() const -> int final { return 3; }
    auto type() const -> Type* final { static Type t; return &t; }
    auto createShader() const -> ShaderIface* final;
    auto createData() const -> ShaderData* final;
    auto updateData(ShaderData *data) -> void final;
    struct SIface; struct SData;
    struct Data;
    Data *d;
};

#endif // TRIANGLEITEM_HPP
