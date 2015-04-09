#include "simplefboitem.hpp"
#include "opengl/openglframebufferobject.hpp"

SimpleFboItem::SimpleFboItem(QQuickItem *parent)
: SimpleTextureItem(parent) {
    connect(&m_sizeChecker, &QTimer::timeout, [this] () {
        if (!_Change(m_prevSize, size().toSize())) {
            m_sizeChecker.stop();
            reserve(UpdateMaterial);
            m_targetSize = m_prevSize;
        }
    });
    m_sizeChecker.setInterval(300);
}

auto SimpleFboItem::finalizeGL() -> void
{
    SimpleTextureItem::finalizeGL(); _Delete(m_fbo);
}

auto SimpleFboItem::updateVertex(Vertex *vertex) -> void
{
    OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex::position,
                                      {0, 0}, {width(), height()});
}

auto SimpleFboItem::geometryChanged(const QRectF &new_, const QRectF &old) -> void
{
    if (m_forced) {
        if (_Change(m_targetSize, new_.size().toSize()))
            {}
        m_forced = false;
    } else
        m_sizeChecker.start();
    reserve(UpdateGeometry);
    if (new_.size() != old.size())
        reserve(UpdateMaterial);
    SimpleTextureItem::geometryChanged(new_, old);
}

auto SimpleFboItem::updateTexture(OpenGLTexture2D *texture) -> void
{
    const auto size = imageSize();
    if (size.isEmpty()) {
        _Delete(m_fbo);
        *texture = OpenGLTexture2D();
    } else {
        if (!m_fbo || m_fbo->size() != size) {
            _Renew(m_fbo, size);
            *texture = m_fbo->texture();
        }
        paint(m_fbo);
    }
}
