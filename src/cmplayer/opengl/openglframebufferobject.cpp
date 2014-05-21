#include "openglframebufferobject.hpp"
#include "opengltexturebinder.hpp"

OpenGLFramebufferObject::OpenGLFramebufferObject(const QSize &size,
                                                 OGL::TextureFormat internal)
    : QOpenGLFramebufferObject(size, NoAttachment, OGL::Target2D, internal)
{
    m_texture.m_id = QOpenGLFramebufferObject::texture();
    m_texture.m_width = size.width();
    m_texture.m_height = size.height();
    m_texture.m_info.texture = internal;
    m_texture.m_info.transfer.format = OGL::RGBA;
    m_texture.m_info.transfer.type = OGL::UInt8;
    if (isValid())
        OpenGLTextureBinder<OGL::Target2D>(&m_texture)->setFilter(OGL::Linear);
}

auto OpenGLFramebufferObject::toImage() const -> QImage
{
    if (!m_texture.isValid())
        return QImage();
    const bool wasBound = isBound();
    if (!wasBound)
        const_cast<OpenGLFramebufferObject*>(this)->bind();
    Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
    QImage image(size(), QImage::Format_ARGB32);
    glReadPixels(0, 0, width(), height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 image.bits());
    if (!wasBound)
        const_cast<OpenGLFramebufferObject*>(this)->release();
    return image;
}
