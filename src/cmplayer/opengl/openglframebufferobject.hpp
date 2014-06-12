#ifndef OPENGLFRAMEBUFFEROBJECT_HPP
#define OPENGLFRAMEBUFFEROBJECT_HPP

#include "opengltexture2d.hpp"

class OpenGLFramebufferObject {
public:
    OpenGLFramebufferObject(const QSize &size, OGL::Target target);
    OpenGLFramebufferObject(const OpenGLTexture2D &texture, bool autodelete = false);
    OpenGLFramebufferObject(const QSize &size, OGL::TextureFormat internal = OGL::RGBA8_UNorm);
    ~OpenGLFramebufferObject();
    const OpenGLTexture2D &texture(int i = 0) const { return m_textures[i]; }
    auto getCoords(double &x1, double &y1, double &x2, double &y2,
                   int idx = 0) const -> void
    {
        auto &tex = m_textures[idx];
        if (tex.target() == OGL::TargetRectangle) {
            x1 = y1 = 0; x2 = tex.width(); y2 = tex.height();
        } else { x1 = y1 = 0; x2 = y2 = 1; }
    }
    auto bind(GLenum target = GL_FRAMEBUFFER) const -> bool;
    auto release() const -> bool;
    auto size() const -> QSize { return m_size; }
    auto width() const -> int { return m_size.width(); }
    auto height() const -> int { return m_size.height(); }
    auto isValid() const -> bool { return m_complete; }
    auto id() const -> GLuint { return m_id; }
    auto attach(const OpenGLTexture2D &texture, int idx = 0) -> bool;
    auto target() const -> OGL::Target { return m_target; }
private:
    static auto func() -> QOpenGLFunctions*
        { return QOpenGLContext::currentContext()->functions(); }
    auto check() const -> bool;
    GLuint m_id = GL_NONE;
    bool m_complete = false, m_autodelete = false;
    QVector<OpenGLTexture2D> m_textures;
    QSize m_size;
    OGL::Target m_target = OGL::Target2D;
};

inline auto OpenGLFramebufferObject::bind(GLenum target) const -> bool
{
    func()->glBindFramebuffer(target, m_id);
    return check();
}

inline auto OpenGLFramebufferObject::release() const -> bool
{
    func()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return check();
}

#endif // OPENGLFRAMEBUFFEROBJECT_HPP
