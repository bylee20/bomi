#ifndef OPENGLTEXTUREBINDER_HPP
#define OPENGLTEXTUREBINDER_HPP

#include "opengltexturebase.hpp"

template<OGL::Target _target>
class OpenGLTextureBinder {
    using trait = OGL::target_trait<_target>;
    using Texture = typename trait::texture_type;
public:
    inline OpenGLTextureBinder() { glGetIntegerv(binding(), &m_restore); }
    inline OpenGLTextureBinder(Texture *texture)
        : OpenGLTextureBinder() { bind(texture); }
    inline ~OpenGLTextureBinder() { glBindTexture(_target, m_restore); }
    constexpr SIA target() -> OGL::Target { return _target; }
    constexpr SIA binding() -> OGL::Binding { return trait::binding; }
    auto bind(Texture *texture) -> void
    {
        Q_ASSERT(texture->target() == _target);
        m_texture = texture; m_texture->bind();
    }
    auto operator * () const -> Texture& { return *m_texture; }
    auto operator -> () const -> Texture* { return m_texture; }
private:
    GLint m_restore = GL_NONE;
    Texture *m_texture = nullptr;
};

class OpenGLTextureBaseBinder {
public:
    inline OpenGLTextureBaseBinder(OGL::Target target)
        : OpenGLTextureBaseBinder(target, OGL::bindingTarget(target)) { }
    inline OpenGLTextureBaseBinder(OGL::Target target, OGL::Binding binding)
        : m_target(target), m_binding(binding)
        { glGetIntegerv(binding, &m_restore); }
    inline ~OpenGLTextureBaseBinder() { glBindTexture(m_target, m_restore); }
    auto target() -> OGL::Target { return m_target; }
    auto binding() -> OGL::Binding { return m_binding; }
    auto bind(OpenGLTextureBase *texture) -> void
    {
        Q_ASSERT(texture->target() == m_target);
        m_texture = texture; m_texture->bind();
    }
    auto operator * () const -> OpenGLTextureBase& { return *m_texture; }
    auto operator -> () const -> OpenGLTextureBase* { return m_texture; }
private:
    GLint m_restore = GL_NONE;
    OGL::Target m_target;
    OGL::Binding m_binding;
    OpenGLTextureBase *m_texture = nullptr;
};

#endif // OPENGLTEXTUREBINDER_HPP
