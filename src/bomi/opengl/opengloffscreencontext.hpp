#ifndef OPENGLOFFSCREENCONTEXT_HPP
#define OPENGLOFFSCREENCONTEXT_HPP

#include <QOpenGLContext>
#include <QOffscreenSurface>

class OpenGLOffscreenContext {
public:
    OpenGLOffscreenContext();
    auto setContextName(const QString &name) -> void;
    auto contextName() const -> QString;
    auto setShareContext(QOpenGLContext *context) -> void;
    auto setThread(QThread *thread) -> void;
    auto setFormat(const QSurfaceFormat &format) -> void;
    auto createContext() -> bool;
    auto createSurface() -> void;
    auto makeCurrent() -> bool { return m_gl.makeCurrent(&m_surface); }
    auto doneCurrent() -> void { m_gl.doneCurrent(); }
    auto context() -> QOpenGLContext* { return &m_gl; }
    auto thread() const -> QThread* { return m_gl.thread(); }
private:
    QOpenGLContext m_gl;
    QOffscreenSurface m_surface;
};

#endif // OPENGLOFFSCREENCONTEXT_HPP
