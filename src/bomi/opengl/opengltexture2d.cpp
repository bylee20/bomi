#include "opengltexture2d.hpp"
#include "opengltexturebinder.hpp"

auto OpenGLTexture2D::initialize(int w, int h, OGL::TransferFormat transfer,
                                 const void *data) -> void
{
    initialize(w, h, OpenGLTextureTransferInfo::get(transfer), data);
}

auto OpenGLTexture2D::toImage() const -> QImage
{
    if (!QOpenGLContext::currentContext())
        return QImage();
    if (isEmpty() || id() == GL_NONE)
        return QImage();
    auto self = const_cast<OpenGLTexture2D*>(this);
    OpenGLTextureBinder<OGL::Target2D> binder(self);
    QImage image(size(), QImage::Format_ARGB32);
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
