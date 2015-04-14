#include "openglmisc.hpp"
#include "misc/log.hpp"
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QSet>
#ifdef Q_OS_LINUX
#include <GL/glx.h>
#endif

DECLARE_LOG_CONTEXT(OpenGL)

namespace OGL {

struct Data {
    bool init = false;
    int major = 0, minor = 0, extensions = 0, maxTexSize = 0;
    QVector<TextureFormat> fboFormats;
};

static Data d;

auto isSupportedFrambufferFormat(TextureFormat format) -> bool
{
    return d.fboFormats.contains(format);
}

auto availableFrambebufferFormats() -> QVector<TextureFormat>
{
    return d.fboFormats;
}

auto is16bitFramebufferFormatSupported() -> bool
{
    return isSupportedFrambufferFormat(RGBA16_UNorm);
}

auto maximumTextureSize() -> int
{
    return d.maxTexSize;
}

auto hasExtension(Extension ext) -> bool
{
    return d.extensions & ext;
}

auto check() -> QString
{
    QOpenGLContext gl;
    if (!gl.create())
        return qApp->translate("OpenGL", "Cannot create OpenGL context!");
    QOffscreenSurface off;
    off.setFormat(gl.format());
    off.create();
    if (!gl.makeCurrent(&off))
        return qApp->translate("OpenGL", "Cannot make OpenGL context current!");

    _Info("Check OpenGL stuffs.");
    const auto version = QOpenGLVersionProfile(gl.format()).version();
    d.major = version.first;
    d.minor = version.second;
    _Info("Version: %%.%%", d.major, d.minor);
    auto versionNumber = [] (int major, int minor)
        { return major*100 + minor; };
    const int current = versionNumber(version.first, version.second);
    if (current < versionNumber(2, 1))
        return qApp->translate("OpenGL", "OpenGL version is too low. "
               "bomi requires OpenGL 2.1 or higher.");

    auto exts = gl.extensions();
    QStringList extensions;
    auto checkExtension = [&] (const QByteArray &name, Extension ext,
                               int major = -1, int minor = 0) {
        if ((major > 0 && current >= versionNumber(major, minor))
                || exts.contains(name)) {
            extensions.append(QString::fromLatin1(name));
            d.extensions |= ext;
        }
    };

    checkExtension("GL_ARB_texture_rg"_b, TextureRG, 3);
    checkExtension("GL_ARB_texture_float"_b, TextureFloat, 3);
    checkExtension("GL_KHR_debug"_b, Debug);
    checkExtension("GL_NV_vdpau_interop"_b, NvVdpauInterop);
    checkExtension("GL_APPLE_ycbcr_422"_b, AppleYCbCr422);
    checkExtension("GL_MESA_ycbcr_texture"_b, MesaYCbCrTexture);
    checkExtension("GLX_EXT_swap_control"_b, ExtSwapControl);
    checkExtension("GLX_SGI_swap_control"_b, SgiSwapControl);
    checkExtension("GLX_MESA_swap_control"_b, MesaSwapControl);

    if (QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        extensions.push_back(u"GL_ARB_framebuffer_object"_q);
        d.extensions |= FramebufferObject;
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &d.maxTexSize);

    if (!hasExtension(FramebufferObject))
        return qApp->translate("OpenGL", "FBO is not available. FBO support is essential.");
    auto testFbo = [] (OGL::TextureFormat format) {
        auto fbo = new QOpenGLFramebufferObject(
            {16, 16}, QOpenGLFramebufferObject::NoAttachment,
            OGL::Target2D, format
        );
        const bool ok = fbo->isValid();
        delete fbo;
        if (ok) {
            d.fboFormats.push_back(format);
            _Info("Available FBO texture format: %%", format);
        }
        return ok;
    };
    testFbo(OGL::RGBA16_UNorm);
    testFbo(OGL::RGBA8_UNorm);
    if (d.fboFormats.isEmpty())
        return qApp->translate("OpenGL", "No available FBO texture format.\n"
               "One of GL_BGRA8 and GL_BGRA16 must be supported at least.");
    _Info("Available extensions: %%", extensions.join(u", "_q));
    return QString();
}

auto errorString(GLenum error) -> const char*
{
    static QHash<GLenum, const char*> strings;
    if (strings.isEmpty()) {
#define ADD(e) {strings[e] = #e;}
        ADD(GL_NO_ERROR);
        ADD(GL_INVALID_ENUM);
        ADD(GL_INVALID_VALUE);
        ADD(GL_INVALID_OPERATION);
        ADD(GL_STACK_OVERFLOW);
        ADD(GL_STACK_UNDERFLOW);
        ADD(GL_OUT_OF_MEMORY);
        ADD(GL_INVALID_FRAMEBUFFER_OPERATION);
        ADD(GL_TABLE_TOO_LARGE);
#undef ADD
    }
    return strings.value(error, "");
}

auto logError(const QByteArray &at) -> int
{
    int num = 0;
    auto error = GL_NO_ERROR;
    while ((error = glGetError()) != GL_NO_ERROR) {
        _Error("Error: %%(0x%%) at %%", errorString(error), error, at);
        ++num;
    }
    return num;
}

}
