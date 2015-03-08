#include "opengltexture2d.hpp"
#include "opengltexturebinder.hpp"

auto OpenGLTexture2D::initialize(const QSize &size, const OpenGLTextureTransferInfo &info,
                                 const void *data) -> void
{
    initialize(size.width(), size.height(), info, data);
}

auto OpenGLTexture2D::initialize(int width, int height, const OpenGLTextureTransferInfo &info,
                                 const void *data) -> void
{
    m_info = info; initialize(width, height, data);
}

auto OpenGLTexture2D::initialize(const QSize &size, const void *data) -> void
{
    initialize(size.width(), size.height(), data);
}

auto OpenGLTexture2D::initialize(int w, int h, OGL::TransferFormat transfer,
                                 const void *data) -> void
{
    initialize(w, h, OpenGLTextureTransferInfo::get(transfer), data);
}

auto OpenGLTexture2D::initialize(int width, int height, const void *data) -> void
{
    m_width = width; m_height = height; initialize(data);
}

auto OpenGLTexture2D::setAttributes(int width, int height, const OpenGLTextureTransferInfo &info) -> void
{
    m_width = width; m_height = height; m_info = info;
}

auto OpenGLTexture2D::initialize(const void *data) -> void
{
    if (!isEmpty())
        glTexImage2D(target(), 0, m_info.texture, m_width, m_height, 0,
                     m_info.transfer.format, m_info.transfer.type, data);
}

auto OpenGLTexture2D::upload(int x, int y, int width, int height, const void *data) -> void
{
    glTexSubImage2D(target(), 0, x, y, width, height, m_info.transfer.format, m_info.transfer.type, data);
}

auto OpenGLTexture2D::upload(const QRect &rect, const void *data) -> void
{
    upload(rect.x(), rect.y(), rect.width(), rect.height(), data);
}

auto OpenGLTexture2D::upload(int width, int height, const void *data) -> void
{
    upload(0, 0, width, height, data);
}

auto OpenGLTexture2D::upload(const void *data) -> void
{
    upload(0, 0, m_width, m_height, data);
}

auto OpenGLTexture2D::toImage(QImage::Format format) const -> QImage
{
    if (!QOpenGLContext::currentContext())
        return QImage();
    if (isEmpty() || id() == GL_NONE)
        return QImage();
    auto self = const_cast<OpenGLTexture2D*>(this);
    OpenGLTextureBinder<OGL::Target2D> binder(self);
    QImage image(size(), format);
    image.fill(0x0);
    glGetTexImage(target(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                  image.bits());
    return image;
}

static QAtomicInt counter;

auto OpenGLTexture2D::save(const QString &fileName) const -> bool
{
    return toImage().save(fileName.arg(counter.fetchAndAddOrdered(1)));
}
