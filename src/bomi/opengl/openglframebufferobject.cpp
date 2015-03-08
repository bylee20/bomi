#include "openglframebufferobject.hpp"
#include "opengltexturebinder.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

auto makeTexture(const QSize &size, OGL::TextureFormat internal) -> OpenGLTexture2D
{
    OpenGLTextureTransferInfo info;
    info.texture = internal;
    info.transfer.format = OGL::BGRA;
    info.transfer.type = OGL::UInt8;

    OpenGLTexture2D texture;
    texture.create(OGL::Linear, OGL::ClampToBorder);
    OpenGLTextureBinder<OGL::Target2D> binder(&texture);
    texture.initialize(size, info);
    return texture;
}

OpenGLFramebufferObject::OpenGLFramebufferObject(const QSize &size,
                                                 OGL::Target target)
    : m_size(size)
    , m_target(target)
{
    func()->glGenFramebuffers(1, &m_id);
}

OpenGLFramebufferObject::OpenGLFramebufferObject(const QSize &size,
                                                 OGL::TextureFormat internal)
    : OpenGLFramebufferObject(makeTexture(size, internal), true) { }

OpenGLFramebufferObject::OpenGLFramebufferObject(const OpenGLTexture2D &texture,
                                                 bool autodelete)
    : OpenGLFramebufferObject(texture.size(), texture.target())
{
    m_autodelete = autodelete;
    m_texture = texture;
    if (texture.isValid() && !texture.isEmpty()) {
        bind();
        attach(texture);
        release();
    }
}

OpenGLFramebufferObject::~OpenGLFramebufferObject()
{
    auto f = func();
    if (m_id != GL_NONE)
        f->glDeleteFramebuffers(1, &m_id);
    if (m_autodelete) {
        m_texture.destroy();
    }
}

auto OpenGLFramebufferObject::attach(const OpenGLTexture2D &texture) -> bool
{
    Q_ASSERT(texture.isValid() && !texture.isEmpty());
    Q_ASSERT(texture.size() == m_size && texture.target() == m_target);
    func()->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   texture.target(), texture.id(), 0);
    m_texture = texture;
    return m_complete = checkStatus();
}

auto OpenGLFramebufferObject::checkStatus() const -> bool
{
    const auto status = func()->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;
    case GL_FRAMEBUFFER_UNDEFINED:
        _Error("FBO Error: GL_FRAMEBUFFER_UNDEFINED");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        _Error("FBO Error: GL_FRAMEBUFFER_UNSUPPORTED");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        _Error("FBO Error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
        break;
    default:
        _Error("FBO Error: unknown error code (0x%%)", _N(status, 16));
        break;
    }
    return false;
}
