#include "triangleitem.hpp"

struct TriangleItem::SData : ShaderData {
    QMatrix4x4 scaler;
};

struct TriangleItem::SIface : ShaderIface {
    SIface() {
        vertexShader = R"(
            uniform mat4 qt_Matrix;
            uniform mat4 scaler;
            attribute vec4 aPosition;
            attribute vec4 aColor;
            varying vec4 color;
            void main() {
                color = aColor;
                gl_Position = scaler * qt_Matrix * aPosition;
            }
        )";
        fragmentShader = R"(
            varying vec4 color;
            uniform float qt_Opacity;
            void main() {
                gl_FragColor = color * qt_Opacity;
            }
        )";
        attributes << "aPosition" << "aColor";
    }
private:
    auto resolve(QOpenGLShaderProgram *prog) -> void final
    {
        loc_scaler = prog->uniformLocation("scaler");
    }
    auto update(QOpenGLShaderProgram *prog, ShaderData *data) -> void final
    {
        auto d = static_cast<const SData*>(data);
        prog->setUniformValue(loc_scaler, d->scaler);
    }
    int loc_scaler = -1;
};

struct TriangleItem::Data {
    QMatrix4x4 scaler;
    QPointF centroid;
};

TriangleItem::TriangleItem(QQuickItem *parent)
    : Super(parent), d(new Data)
{
    vertices().resize(3);
    polish();
}

TriangleItem::~TriangleItem()
{
    delete d;
}

static auto operator != (const OGL::CoordAttr &attr, const QPointF &p) -> bool
{
    return attr.x != (float)p.x() || attr.y != (float)p.y();
}

static auto operator != (const OGL::ColorAttr &attr, const QColor &c) -> bool
{
    return attr.a != c.alpha() || attr.g != c.green()
            || attr.b != c.blue() || attr.a != c.alpha();
}

auto TriangleItem::p1() const -> QPointF
{
    return vertices()[0].position.toPoint();
}

auto TriangleItem::p2() const -> QPointF
{
    return vertices()[1].position.toPoint();
}

auto TriangleItem::p3() const -> QPointF
{
    return vertices()[2].position.toPoint();
}

template<class Attr, class Value>
static auto checkUpdate(Attr &attr, const Value &value) -> bool
{
    if (attr != value) {
        attr.set(value);
        return true;
    }
    return false;
}

auto TriangleItem::setP1(const QPointF &p) -> void
{
    if (checkUpdate(vertices()[0].position, p)) {
        emit p1Changed(p);
        reserve(UpdateGeometry);
        polish();
    }
}

auto TriangleItem::setP2(const QPointF &p) -> void
{
    if (checkUpdate(vertices()[1].position, p)) {
        emit p2Changed(p);
        reserve(UpdateGeometry);
        polish();
    }
}
auto TriangleItem::setP3(const QPointF &p) -> void
{
    if (checkUpdate(vertices()[2].position, p)) {
        emit p3Changed(p);
        reserve(UpdateGeometry);
        polish();
    }
}

auto TriangleItem::color() const -> QColor
{
    return vertices()[0].color.toColor();
}

auto TriangleItem::setColor(const QColor &color) -> void
{
    if (checkUpdate(vertices()[0].color, color)) {
        vertices()[1].color.set(color);
        vertices()[2].color.set(color);
        emit colorChanged(color);
        reserve(UpdateMaterial);
    }
}

auto TriangleItem::createShader() const -> ShaderIface*
{
    return new SIface;
}

auto TriangleItem::createData() const -> ShaderData*
{
    return new SData;
}

auto TriangleItem::updateData(ShaderData *data) -> void
{
    static_cast<SData*>(data)->scaler = d->scaler;
}

auto TriangleItem::geometryChanged(const QRectF &new_, const QRectF &old) -> void
{
    Super::geometryChanged(new_, old);
    if (new_.size() != old.size())
        polish();
}

auto TriangleItem::updatePolish() -> void
{
    d->scaler.setToIdentity();
    d->scaler.scale(width(), height());
    reserve(UpdateAll);
}
