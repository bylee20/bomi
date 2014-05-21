#ifndef OPENGLFRAMEBUFFEROBJECT_HPP
#define OPENGLFRAMEBUFFEROBJECT_HPP

#include "opengltexture2d.hpp"

class OpenGLFramebufferObject : public QOpenGLFramebufferObject {
public:
    OpenGLFramebufferObject(const QSize &size,
                            OGL::TextureFormat internal = OGL::RGBA8_UNorm);
    const OpenGLTexture2D &texture() const { return m_texture; }
    auto getCoords(double &x1, double &y1, double &x2, double &y2) -> void
    {
        if (m_texture.target() == OGL::TargetRectangle) {
            x1 = y1 = 0; x2 = m_texture.width(); y2 = m_texture.height();
        } else { x1 = y1 = 0; x2 = y2 = 1; }
    }
    auto toImage() const -> QImage;
private:
    OpenGLTexture2D m_texture;
};

#endif // OPENGLFRAMEBUFFEROBJECT_HPP
