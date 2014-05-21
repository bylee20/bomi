#include "opengltexturebase.hpp"
#include "opengltexturebinder.hpp"

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
