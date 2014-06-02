#ifndef OPENGLFRAMEBUFFEROBJECT_HPP
#define OPENGLFRAMEBUFFEROBJECT_HPP

#include "opengltexture2d.hpp"

class OpenGLFramebufferObject {
public:
    OpenGLFramebufferObject(const OpenGLTexture2D &texture, bool autodelete = false);
    OpenGLFramebufferObject(const QSize &size, OGL::TextureFormat internal = OGL::RGBA8_UNorm);
    ~OpenGLFramebufferObject();
    const OpenGLTexture2D &texture() const { return m_texture; }
    auto getCoords(double &x1, double &y1, double &x2, double &y2) -> void
    {
        if (m_texture.target() == OGL::TargetRectangle) {
            x1 = y1 = 0; x2 = m_texture.width(); y2 = m_texture.height();
        } else { x1 = y1 = 0; x2 = y2 = 1; }
    }
//    auto toImage() const -> QImage;
    auto bind(GLenum target = GL_FRAMEBUFFER) const -> bool;
    auto release() const -> bool;
    auto size() const -> QSize { return m_texture.size(); }
    auto width() const -> int { return m_texture.width(); }
    auto height() const -> int { return m_texture.height(); }
    auto isValid() const -> bool { return m_complete; }
    auto id() const -> GLuint { return m_id; }
private:
    static auto func() -> QOpenGLFunctions*
        { return QOpenGLContext::currentContext()->functions(); }
    auto check() const -> bool;
    GLuint m_id = GL_NONE;
    bool m_complete = false, m_autodelete = false;
    OpenGLTexture2D m_texture;
};

inline auto OpenGLFramebufferObject::bind(GLenum target) const -> bool
{
    if (!m_complete)
        return false;
    func()->glBindFramebuffer(target, m_id);
    return check();
}

inline auto OpenGLFramebufferObject::release() const -> bool
{
    auto ctx = QOpenGLContext::currentContext();
    func()->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
    return check();
}


//class OpenGLFramebufferObject {
//public:
//OpenGLFramebufferObject(const QSize &size, int target = GL_TEXTURE_2D)
//: OpenGLFramebufferObject(size, OpenGLCompat::textureFormat(GL_BGRA), target) {}
//OpenGLFramebufferObject(const QSize &size, const OpenGLTextureFormat &format, int target = GL_TEXTURE_2D) {
//auto f = func();
//m_texture.width = size.width();
//m_texture.height = size.height();
//m_texture.target = target;
//m_texture.format = format;
//if (!m_texture.isEmpty()) {
//f->glGenFramebuffers(1, &m_id);
//LOG_GL_ERROR
//f->glBindFramebuffer(GL_FRAMEBUFFER, m_id);
//LOG_GL_ERROR
//m_texture.generate();
//m_texture.allocate();
//f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texture.id, 0);
//            m_complete = f->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
//LOG_GL_ERROR
//QOpenGLFramebufferObject::bindDefault();
//}
//}
//virtual ~OpenGLFramebufferObject() {
//m_texture.delete_();
//if (m_id != GL_NONE)
//func()->glDeleteFramebuffers(1, &m_id);
//}
//    bool isComplete() const { return m_complete; }
//QRect rect() const { return {0, 0, width(), height()}; }
//int width() const { return m_texture.width; }
//int height() const { return m_texture.height; }
//QSize size() const { return {m_texture.width, m_texture.height}; }
//    void bind() const { if (m_complete) func()->glBindFramebuffer(GL_FRAMEBUFFER, m_id); }
//void release() const { QOpenGLFramebufferObject::bindDefault(); }
//const OpenGLTexture &texture() const { return m_texture; }
//QImage toImage() const;
//void getCoords(double &x1, double &y1, double &x2, double &y2) {
//if (m_texture.target == GL_TEXTURE_RECTANGLE) {
//x1 = y1 = 0; x2 = m_texture.width; y2 = m_texture.height;
//} else { x1 = y1 = 0; x2 = y2 = 1; }
//}
//private:
//static QOpenGLFunctions *func() { return OpenGLCompat::functions(); }
//GLuint m_id = GL_NONE;
//OpenGLTexture m_texture;
//    bool m_complete = false;
//};


#endif // OPENGLFRAMEBUFFEROBJECT_HPP
