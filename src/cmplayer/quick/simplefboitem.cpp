#include "simplefboitem.hpp"

SimpleFboItem::SimpleFboItem(QQuickItem *parent)
: SimpleTextureItem(parent) {
    connect(&m_sizeChecker, &QTimer::timeout, [this] () {
        if (!_Change(m_prevSize, QSizeF(width(), height()).toSize())) {
            m_sizeChecker.stop();
            emit targetSizeChanged(m_targetSize = m_prevSize);
        }
    });
    m_sizeChecker.setInterval(300);
}

void SimpleFboItem::updateVertex(Vertex *vertex) {
    OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex::position,
                                      {0, 0}, {width(), height()});
}

void SimpleFboItem::geometryChanged(const QRectF &new_, const QRectF &old) {
    if (m_forced) {
        if (_Change(m_targetSize, new_.size().toSize()))
            emit targetSizeChanged(m_targetSize);
        m_forced = false;
    }else
        m_sizeChecker.start();
    reserve(UpdateAll);
    SimpleTextureItem::geometryChanged(new_, old);
}

void SimpleFboItem::updateTexture(OpenGLTexture2D *texture) {
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
