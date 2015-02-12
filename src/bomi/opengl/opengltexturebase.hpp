#ifndef OPENGLTEXTUREBASE_HPP
#define OPENGLTEXTUREBASE_HPP

#include "openglmisc.hpp"
#include "opengltexturetransferinfo.hpp"
#include <QOpenGLShaderProgram>

class OpenGLTextureBase {
public:
    virtual ~OpenGLTextureBase() {}
    auto id() const -> GLuint { return m_id; }
    auto target() const -> OGL::Target { return m_target; }
    auto create(OGL::Filter f, OGL::WrapMode wrap = OGL::ClampToEdge) -> void;
    auto create(OGL::WrapMode wrap, OGL::Filter filter = OGL::Linear) -> void
    { create(filter, wrap); }
    auto create() -> void { create(OGL::Linear, OGL::ClampToEdge); }
    auto destroy() -> void { glDeleteTextures(1, &m_id); m_id = GL_NONE; }
    auto bind() const -> void { glBindTexture(m_target, m_id); }
    auto bind(QOpenGLShaderProgram *prog, int location, int index) const -> void
    {
        prog->setUniformValue(location, index);
        OGL::func()->glActiveTexture(GL_TEXTURE0 + index);
        bind();
    }
    auto setFilter(OGL::Filter filter) -> void;
    auto setWrapMode(OGL::WrapMode wrap) -> void;
    auto isValid() const -> bool { return m_id != GL_NONE; }
    const OpenGLTextureTransferInfo &info() const { return m_info; }
    auto format() const -> OGL::TextureFormat { return m_info.texture; }
    const OGL::TransferInfo &transfer() const { return m_info.transfer; }
protected:
    OpenGLTextureBase(OGL::Target target): m_target(target) {}
    OpenGLTextureTransferInfo m_info;
private:
    GLuint m_id = GL_NONE;
    friend class OpenGLFramebufferObject;
    OGL::Target m_target = OGL::Target2D;
};

#endif // OPENGLTEXTUREBASE_HPP
