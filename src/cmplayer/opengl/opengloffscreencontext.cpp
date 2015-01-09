#include "opengloffscreencontext.hpp"

OpenGLOffscreenContext::OpenGLOffscreenContext()
{
    setContextName(u"OpenGLOffscreenContext"_q);
}

auto OpenGLOffscreenContext::setFormat(const QSurfaceFormat &format) -> void
{
    m_gl.setFormat(format);
    m_surface.setFormat(format);
}

auto OpenGLOffscreenContext::setShareContext(QOpenGLContext *context) -> void
{
    m_gl.setShareContext(context);
}

auto OpenGLOffscreenContext::setThread(QThread *thread) -> void
{
    Q_ASSERT(m_gl.thread() == QThread::currentThread());
    m_gl.moveToThread(thread);
}

auto OpenGLOffscreenContext::createContext() -> bool
{
    return m_gl.create();
}

auto OpenGLOffscreenContext::createSurface() -> void
{
    m_surface.create();
}

auto OpenGLOffscreenContext::setContextName(const QString &name) -> void
{
    m_gl.setObjectName(name);
}

auto OpenGLOffscreenContext::contextName() const -> QString
{
    return m_gl.objectName();
}
