#include "circularimageitem.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"

struct CircularImageItem::Data {
    QUrl url;
    QSize imageSize;
    bool load = false;
    double radian = 2*M_PI;
    auto normalize(double value) -> double
    {
        static const constexpr double start = 0;
        static const constexpr double end = 360;
        static const constexpr double width = end - start;
        const double offset = value - start ;   // value relative to 0
        return (offset - (floor(offset / width) * width)) + start;
    }
};

CircularImageItem::CircularImageItem(QQuickItem *parent)
    : SimpleTextureItem(parent), d(new Data)
{
    setVisible(true);

}

CircularImageItem::~CircularImageItem()
{
    delete d;
}

auto CircularImageItem::initializeGL() -> void
{
    SimpleTextureItem::initializeGL();
    auto &tex = texture();
    tex.create();
    tex.setAttributes(0, 0, OpenGLTextureTransferInfo::get(OGL::BGRA));
}

auto CircularImageItem::finalizeGL() -> void
{
    SimpleTextureItem::finalizeGL();
    texture().destroy();
}

auto CircularImageItem::angle() const -> qreal
{
    return d->radian;
}

auto CircularImageItem::setAngle(qreal rad) -> void
{
    if (_Change(d->radian, rad)) {
        reserve(UpdateGeometry);
        emit angleChanged();
    }
}

auto CircularImageItem::source() const -> QUrl
{
    return d->url;
}

auto CircularImageItem::setSource(const QUrl &url) -> void
{
    if (_Change(d->url, url)) {
        d->load = true;
        reserve(UpdateAll);
        emit sourceChanged(d->url);
    }
}

auto CircularImageItem::updateVertex(Vertex *vertex) -> void
{
//    if (d->radian2 < d->radian1)
//        return;
    const auto r1 = d->normalize(d->radian);
    OGL::TextureVertex *it = vertex;
    QSizeF s(1, 1); s.scale(width(), height(), Qt::KeepAspectRatio);
    const auto rp = 2*std::sqrt(width()*width() + height()*height())*0.5;
    const auto rt = 2*std::sqrt(0.5*0.5 + 0.5*0.5);
    const double d = s.width()*0.5;
    const QPointF origin((width() - s.width())*0.5, (height() - s.height())*0.5);
    auto put = [&] (double theta1, double theta2) {
        const auto sin1 = std::sin(theta1);
        const auto sin2 = std::sin(theta2);
        const auto cos1 = std::cos(theta1);
        const auto cos2 = std::cos(theta2);
        auto set = [&] (double sin, double cos) {
            it->position.x = rp*cos + d + origin.x();
            it->position.y = rp*sin + d + origin.y();
            it->texCoord.x = rt*cos + .5;
            it->texCoord.y = rt*sin + .5;
            ++it;
        };
        set(0, 0);
        set(sin1, cos1);
        set(sin2, cos2);
    };
    if (_InRange(0., r1, M_PI/4)) {
        put(0, r1);
    } else {
        put(0, M_PI/4);
        if (_InRange(M_PI/4, r1, M_PI*3/4)) {
            put(M_PI/4, r1);
        } else {
            put(M_PI/4, M_PI*3/4);
            if (_InRange(M_PI*3/4, r1, M_PI*5/4))
                put(M_PI*3/4, r1);
            else {
                put(M_PI*3/4, M_PI*5/4);
                if (_InRange(M_PI*5/4, r1, M_PI*7/4))
                    put(M_PI*5/4, r1);
                else {
                    put(M_PI*5/4, M_PI*7/4);
                    put(M_PI*7/4, r1);
                }
            }
        }
    }
    const auto zeros = vertex + vertexCount() - it;
    memset(it, 0, sizeof(Vertex)*zeros);
}

auto CircularImageItem::geometryChanged(const QRectF &new_, const QRectF &old) -> void
{
    SimpleTextureItem::geometryChanged(new_, old);
}

auto CircularImageItem::drawingMode() const -> GLenum
{
    return GL_TRIANGLES;
}

auto CircularImageItem::updateVertexOnGeometryChanged() const -> bool
{
    return true;
}

auto CircularImageItem::vertexCount() const -> int
{
    return 5*3;
}

auto CircularImageItem::updateTexture(OpenGLTexture2D *texture) -> void
{
    if (d->load) {
        QImage image;
        d->imageSize = QSize();
        if (!image.load(d->url.toLocalFile()))
            return;
        OpenGLTextureBinder<OGL::Target2D> binder(texture);
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        d->imageSize = image.size();
        texture->initialize(d->imageSize, image.bits());
    }
}
