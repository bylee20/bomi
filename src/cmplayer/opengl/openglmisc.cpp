#include "openglmisc.hpp"
#include "openglcompat.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

auto OpenGLTexture2D::initialize(int width, int height, OGL::TransferFormat transfer, const void *data) -> void
{
    initialize(width, height, OpenGLCompat::transferInfo(transfer), data);
}

auto OpenGLTextureBase::setFilter(OGL::Filter filter) -> void
{
    glTexParameterf(m_target, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameterf(m_target, GL_TEXTURE_MIN_FILTER, filter);
}

auto OpenGLTextureBase::setWrapMode(OGL::WrapMode wrap) -> void
{
    switch (m_target) {
    case GL_TEXTURE_3D:
        glTexParameterf(m_target, GL_TEXTURE_WRAP_R, wrap);
    case OGL::Target2D:
    case OGL::TargetRectangle:
        glTexParameterf(m_target, GL_TEXTURE_WRAP_T, wrap);
    case OGL::Target1D:
        glTexParameterf(m_target, GL_TEXTURE_WRAP_S, wrap);
        break;
    default:
        break;
    }
}

auto OpenGLTextureBase::create(OGL::Filter filter, OGL::WrapMode wrap) -> void
{
    glGenTextures(1, &m_id);
    OpenGLTextureBaseBinder binder(m_target, OGL::bindingTarget(m_target));
    binder.bind(this);
    setFilter(filter);
    setWrapMode(wrap);
}

auto OpenGLTexture2D::toImage() const -> QImage
{
    if (isEmpty() || id() == GL_NONE)
        return QImage();
    OpenGLTextureBinder<OGL::Target2D> binder(const_cast<OpenGLTexture2D*>(this));
    QImage image(size(), QImage::Format_ARGB32);
    image.fill(0x0);
    glGetTexImage(target(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits());
    return image;
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
    glReadPixels(0, 0, width(), height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits());
    if (!wasBound)
        const_cast<OpenGLFramebufferObject*>(this)->release();
    return image;
}
