#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include "stdafx.hpp"
#include "openglmisc.hpp"

class OpenGLTextureTransferInfo;

class OpenGLCompat {
    using TransferInfo = OpenGLTextureTransferInfo;
public:
    enum Extension {
        TextureRG         = 1 << 0,
        TextureFloat      = 1 << 1,
        Debug             = 1 << 2,
        NvVdpauInterop    = 1 << 3,
        FramebufferObject = 1 << 4,
        AppleYCbCr422     = 1 << 5,
        MesaYCbCrTexture  = 1 << 6,
        ExtSwapControl    = 1 << 7,
        SgiSwapControl    = 1 << 8,
        MesaSwapControl   = 1 << 9
    };
    ~OpenGLCompat();
    static auto initialize(QOpenGLContext *ctx) -> void;
    static auto finalize(QOpenGLContext *ctx) -> void;
    static auto transferInfo(OGL::TransferFormat format,
                             int bytesPerComponent = 1) -> TransferInfo;
    static auto rg(const char *rg) -> QByteArray;
    static auto maximumTextureSize() -> int { return s_maxTexSize; }
    static auto hasExtension(Extension ext) -> bool
        { return s_extensions & ext; }
    static auto logError(const char *at) -> int;
    static auto check() -> void;
    static auto errorString(GLenum error) -> const char*;
    static auto logger() -> QOpenGLDebugLogger*;
    static auto framebufferObjectTextureFormat() -> OGL::TextureFormat;
    static auto debug(const QOpenGLDebugMessage &message) -> void;
private:
    OpenGLCompat();
    static int s_maxTexSize, s_extensions;
    static OpenGLCompat s;
    struct Data;
    Data *d;
};

inline auto OpenGLCompat::rg(const char *rg) -> QByteArray
{
    if (hasExtension(TextureRG))
        return QByteArray(rg);
    return QByteArray(rg).replace('g', 'a');
}

static inline auto _GLFunc() -> QOpenGLFunctions* {
    auto ctx = QOpenGLContext::currentContext();
    return ctx ? ctx->functions() : nullptr;
}

#endif // OPENGLCOMPAT_HPP
