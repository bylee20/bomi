#ifndef OPENGLOFFSCREENCONTEXT_HPP
#define OPENGLOFFSCREENCONTEXT_HPP

#include "stdafx.hpp"

class OpenGLOffscreenContext {
public:
    OpenGLOffscreenContext();
    auto setShareContext(QOpenGLContext *context) -> void;
    auto setThread(QThread *thread) -> void;
    auto setFormat(const QSurfaceFormat &format) -> void;
    auto create() -> bool;
    auto makeCurrent() -> bool { return m_gl.makeCurrent(&m_surface); }
    auto doneCurrent() -> void { m_gl.doneCurrent(); }
private:
    QOpenGLContext m_gl;
    QOffscreenSurface m_surface;
};

#endif // OPENGLOFFSCREENCONTEXT_HPP
