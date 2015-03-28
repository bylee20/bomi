#ifndef OPENGLFRAMEBUFFEROBJECT_HPP
#define OPENGLFRAMEBUFFEROBJECT_HPP

#include "opengltexture2d.hpp"

class OpenGLFramebufferObject {
public:
    OpenGLFramebufferObject(const QSize &size, OGL::Target target);
    OpenGLFramebufferObject(const OpenGLTexture2D &texture, bool autodelete = false);
    OpenGLFramebufferObject(const QSize &size, OGL::TextureFormat internal = OGL::RGBA8_UNorm);
    ~OpenGLFramebufferObject();
    const OpenGLTexture2D &texture() const { return m_texture; }
    auto getCoords(double &x1, double &y1, double &x2, double &y2) const -> void
    {
        if (m_texture.target() == OGL::TargetRectangle) {
            x1 = y1 = 0; x2 = m_texture.width(); y2 = m_texture.height();
        } else { x1 = y1 = 0; x2 = y2 = 1; }
    }
    auto format() const { return m_texture.format(); }
    auto bind(GLenum target = GL_FRAMEBUFFER) const -> void;
    auto release() const -> void;
    auto size() const -> QSize { return m_size; }
    auto width() const -> int { return m_size.width(); }
    auto height() const -> int { return m_size.height(); }
    auto isValid() const -> bool { return m_complete; }
    auto id() const -> GLuint { return m_id; }
    auto attach(const OpenGLTexture2D &texture) -> bool;
    auto target() const -> OGL::Target { return m_target; }
    auto checkStatus() const -> bool;
private:
    static auto func() -> QOpenGLFunctions*
        { return QOpenGLContext::currentContext()->functions(); }
    GLuint m_id = GL_NONE;
    bool m_complete = false, m_autodelete = false;
    OpenGLTexture2D m_texture;
    QSize m_size;
    OGL::Target m_target = OGL::Target2D;
};

inline auto OpenGLFramebufferObject::bind(GLenum target) const -> void
{
    func()->glBindFramebuffer(target, m_id);
}

inline auto OpenGLFramebufferObject::release() const -> void
{
    func()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif // OPENGLFRAMEBUFFEROBJECT_HPP
