#include "openglcompat.hpp"
#include "app.hpp"
#include <cmath>
#include "log.hpp"
#ifdef Q_OS_LINUX
#include <GL/glx.h>
#endif
#include "openglframebufferobject.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

OpenGLCompat OpenGLCompat::s;
int OpenGLCompat::s_maxTexSize = 0;
int OpenGLCompat::s_extensions = 0;

struct OpenGLCompat::Data {
    bool init = false;
    QOpenGLDebugLogger *logger = nullptr;
    int major = 0, minor = 0;
    QMap<OGL::TransferFormat, OpenGLTextureTransferInfo> formats[2];
    OGL::TextureFormat fboFormat = OGL::RGBA8_UNorm;
};

OpenGLCompat::OpenGLCompat()
: d(new Data) {
}

OpenGLCompat::~OpenGLCompat() {
    delete d->logger;
    delete d;
}

auto OpenGLCompat::logger() -> QOpenGLDebugLogger*
{
    return s.d->logger;
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Source source) {
    switch (source) {
#define SWITCH_SOURCE(s) case QOpenGLDebugMessage::s##Source: return #s;
    SWITCH_SOURCE(API)                SWITCH_SOURCE(Invalid)
    SWITCH_SOURCE(WindowSystem)        SWITCH_SOURCE(ShaderCompiler)
    SWITCH_SOURCE(ThirdParty)        SWITCH_SOURCE(Application)
    SWITCH_SOURCE(Other)            SWITCH_SOURCE(Any)
#undef SWITCH_SOURCE
    }
    return QByteArray::number(source, 16);
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Type type) {
    switch (type) {
#define SWITCH_TYPE(t) case QOpenGLDebugMessage::t##Type: return #t;
    SWITCH_TYPE(Invalid)            SWITCH_TYPE(Error)
    SWITCH_TYPE(DeprecatedBehavior)    SWITCH_TYPE(UndefinedBehavior)
    SWITCH_TYPE(Portability)        SWITCH_TYPE(Performance)
    SWITCH_TYPE(Other)                SWITCH_TYPE(Marker)
    SWITCH_TYPE(GroupPush)            SWITCH_TYPE(GroupPop)
    SWITCH_TYPE(Any)
#undef SWITCH_TYPE
    }
    return QByteArray::number(type, 16);
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Severity severity) {
    switch (severity) {
#define SWITCH_SEVERITY(s) case QOpenGLDebugMessage::s##Severity: return #s;
    SWITCH_SEVERITY(Invalid)        SWITCH_SEVERITY(High)
    SWITCH_SEVERITY(Medium)            SWITCH_SEVERITY(Low)
    SWITCH_SEVERITY(Notification)    SWITCH_SEVERITY(Any)
#undef SWITCH_SEVERITY
    }
    return QByteArray::number(severity, 16);
}

auto OpenGLCompat::debug(const QOpenGLDebugMessage &message) -> void
{
    if (message.type() == QOpenGLDebugMessage::ErrorType)
        _Error("Error: %%", message.message().trimmed());
    else
        _Debug("Logger: %% (%%/%%/%%)", message.message().trimmed(),
               message.source(), message.severity(), message.type());
}

auto OpenGLCompat::errorString(GLenum error) -> const char*
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

auto OpenGLCompat::logError(const char *at) -> int
{
    int num = 0;
    auto error = GL_NO_ERROR;
    while ((error = glGetError()) != GL_NO_ERROR) {
        _Error("Error: %%(0x%%) at %%", errorString(error), error, at);
        ++num;
    }
    return num;
}

auto OpenGLCompat::framebufferObjectTextureFormat() -> OGL::TextureFormat
{
    return s.d->fboFormat;
}

auto OpenGLCompat::check() -> void
{
    QOpenGLContext gl;
    if (!gl.create())
        _Fatal("Cannot create OpenGL context!");
    QOffscreenSurface off;
    off.setFormat(gl.format());
    off.create();
    if (!gl.makeCurrent(&off))
        _Fatal("Cannot make OpenGL context current!");

    auto d = s.d;

    _Info("Check OpenGL stuffs.");
    const auto version = QOpenGLVersionProfile(gl.format()).version();
    d->major = version.first;
    d->minor = version.second;
    _Info("Version: %%.%%", d->major, d->minor);
    auto versionNumber = [] (int major, int minor)
        { return major*100 + minor; };
    const int current = versionNumber(version.first, version.second);
    if (current < versionNumber(2, 1))
        _Fatal("OpenGL version is too low. "
               "CMPlayer requires OpenGL 2.1 or higher.");

    auto exts = gl.extensions();
#ifdef Q_OS_LINUX
    const auto glx = glXQueryExtensionsString(QX11Info::display(),
                                              QX11Info::appScreen());
    exts += QSet<QByteArray>::fromList(QByteArray(glx).split(' '));
#endif
    QStringList extensions;
    auto checkExtension = [&] (const char *name, Extension ext,
                               int major = -1, int minor = 0) {
        if ((major > 0 && current >= versionNumber(major, minor))
                || exts.contains(name)) {
            extensions.append(name);
            s_extensions |= ext;
        }
    };

    checkExtension("GL_ARB_texture_rg", TextureRG, 3);
    checkExtension("GL_ARB_texture_float", TextureFloat, 3);
    checkExtension("GL_KHR_debug", Debug);
    checkExtension("GL_NV_vdpau_interop", NvVdpauInterop);
    checkExtension("GL_APPLE_ycbcr_422", AppleYCbCr422);
    checkExtension("GL_MESA_ycbcr_texture", MesaYCbCrTexture);
    checkExtension("GLX_EXT_swap_control", ExtSwapControl);
    checkExtension("GLX_SGI_swap_control", SgiSwapControl);
    checkExtension("GLX_MESA_swap_control", MesaSwapControl);

    if (QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        extensions.append("GL_ARB_framebuffer_object");
        s_extensions |= FramebufferObject;
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s_maxTexSize);

    d->formats[0][OGL::Red] = {OGL::R8_UNorm, OGL::Red, OGL::UInt8};
    d->formats[0][OGL::RG] = {OGL::RG8_UNorm, OGL::RG, OGL::UInt8};
    d->formats[0][OGL::Luminance] = {OGL::Luminance8_UNorm,
                                     OGL::Luminance, OGL::UInt8};
    d->formats[0][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha8_UNorm,
                                          OGL::LuminanceAlpha, OGL::UInt8};
    d->formats[0][OGL::RGB] = {OGL::RGB8_UNorm, OGL::RGB, OGL::UInt8};
    d->formats[0][OGL::BGR] = {OGL::RGB8_UNorm, OGL::BGR, OGL::UInt8};
    d->formats[0][OGL::BGRA] = {OGL::RGBA8_UNorm,
                                OGL::BGRA, OGL::UInt32_8_8_8_8_Rev};
    d->formats[0][OGL::RGBA] = {OGL::RGBA8_UNorm,
                                OGL::RGBA, OGL::UInt32_8_8_8_8_Rev};

    d->formats[1][OGL::Red] = {OGL::R16_UNorm, OGL::Red, OGL::UInt16};
    d->formats[1][OGL::RG] = {OGL::R16_UNorm, OGL::RG, OGL::UInt16};
    d->formats[1][OGL::Luminance] = {OGL::Luminance16_UNorm,
                                     OGL::Luminance, OGL::UInt16};
    d->formats[1][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha16_UNorm,
                                          OGL::LuminanceAlpha, OGL::UInt16};
    d->formats[1][OGL::RGB] = {OGL::RGB16_UNorm, OGL::RGB, OGL::UInt16};
    d->formats[1][OGL::BGR] = {OGL::RGB16_UNorm, OGL::BGR, OGL::UInt16};
    d->formats[1][OGL::BGRA] = {OGL::RGBA16_UNorm, OGL::BGRA, OGL::UInt16};
    d->formats[1][OGL::RGBA] = {OGL::RGBA16_UNorm, OGL::RGBA, OGL::UInt16};

    const bool rg = hasExtension(TextureRG);
    for (auto &format : d->formats) {
        if (rg) {
            format[OGL::OneComponent] = format[OGL::Red];
            format[OGL::TwoComponents] = format[OGL::RG];
        } else {
            format[OGL::OneComponent] = format[OGL::Luminance];
            format[OGL::TwoComponents] = format[OGL::LuminanceAlpha];
        }
    }

    if (!hasExtension(FramebufferObject))
        _Fatal("FBO is not available. FBO support is essential.");
    auto fbo = new OpenGLFramebufferObject(QSize(16, 16), OGL::RGBA16_UNorm);
    if (fbo->isValid()) {
        d->fboFormat = OGL::RGBA8_UNorm;
        _Info("FBO texture format: GL_RGBA16");
    } else {
        if (!_Renew(fbo, QSize(16, 16), OGL::RGBA8_UNorm)->isValid())
            _Fatal("No available FBO texture format.\n"
                   "One of GL_BGRA8 and GL_BGRA16 must be supported at least.");
        else
            _Info("FBO texture format: OGL::RGBA8_UNorm");
    }
    _Delete(fbo);
    if (!extensions.isEmpty())
        _Info("Available extensions: %%", extensions.join(", "));
}

auto OpenGLCompat::finalize(QOpenGLContext */*ctx*/) -> void
{
    auto d = s.d;
    if (d->init) {
        if (d->logger && d->logger->isLogging())
            d->logger->stopLogging();
        _Delete(d->logger);
        d->init = false;
    }
}

auto OpenGLCompat::initialize(QOpenGLContext *ctx) -> void
{
    auto d = s.d;
    if (d->init)
        return;
    d->init = true;
    if (!cApp.isOpenGLDebugLoggerRequested())
        return;
    if (hasExtension(Debug)
            && ctx->format().testOption(QSurfaceFormat::DebugContext)) {
        d->logger = new QOpenGLDebugLogger;
        if (!d->logger->initialize()) {
            logError("OpenGLCompat::initialize()");
            delete d->logger;
        } else
            _Debug("OpenGL debug logger is running.");
    } else
        _Error("OpenGL debug logger was requested but it is not supported.");
}

auto OpenGLCompat::transferInfo(OGL::TransferFormat format,
                                int bytesPerComponent) -> TransferInfo
{
    Q_ASSERT(bytesPerComponent == 1 || bytesPerComponent == 2);
    return s.d->formats[bytesPerComponent-1][format];
}
