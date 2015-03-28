#ifndef OPENGLMISC_HPP
#define OPENGLMISC_HPP

#ifdef PixelFormat
#undef PixelFormat
#endif

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define GL_UNSIGNED_SHORT_8_8_MESA      0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA  0x85BB
#endif

#include <QOpenGLFunctions>

class OpenGLTextureBase;
class OpenGLTexture1D;
class OpenGLTexture2D;
class OpenGLTexture3D;

namespace OGL {

enum Target {
    Target1D        = GL_TEXTURE_1D,
    Target2D        = GL_TEXTURE_2D,
    Target3D        = GL_TEXTURE_3D,
    TargetRectangle = GL_TEXTURE_RECTANGLE
};
enum Binding {
    Binding1D = GL_TEXTURE_BINDING_1D,
    Binding2D = GL_TEXTURE_BINDING_2D,
    Binding3D = GL_TEXTURE_BINDING_3D,
    BindingRectangle = GL_TEXTURE_BINDING_RECTANGLE,
};

SIA bindingTarget(Target target) -> Binding {
    switch (target) {
    case Target1D: return Binding1D;
    case Target2D: return Binding2D;
    case Target3D: return Binding3D;
    case TargetRectangle: return BindingRectangle;
    default: return (Binding)GL_NONE;
    }
}

template<Target target>
struct target_trait { using texture_type = OpenGLTextureBase; };
template<> struct target_trait<Target1D> {
    static constexpr Target target = Target1D;
    static constexpr Binding binding = Binding1D;
    using texture_type = OpenGLTexture1D;
};
template<> struct target_trait<Target2D> {
    static constexpr Target target = Target2D;
    static constexpr Binding binding = Binding2D;
    using texture_type = OpenGLTexture2D;
};
template<> struct target_trait<Target3D> {
    static constexpr Target target = Target3D;
    static constexpr Binding binding = Binding3D;
    using texture_type = OpenGLTexture3D;
};
template<> struct target_trait<TargetRectangle> {
    static constexpr Target target = TargetRectangle;
    static constexpr Binding binding = BindingRectangle;
    using texture_type = OpenGLTexture2D;
};

enum TransferType { // pixel format
    NoTransferType       = GL_NONE,
    UInt8                = GL_UNSIGNED_BYTE,
    UInt16               = GL_UNSIGNED_SHORT,
    Float32              = GL_FLOAT,
    UInt32_8_8_8_8       = GL_UNSIGNED_INT_8_8_8_8,
    UInt32_8_8_8_8_Rev   = GL_UNSIGNED_INT_8_8_8_8_REV,
    UInt16_8_8_Apple     = GL_UNSIGNED_SHORT_8_8_APPLE,
    UInt16_8_8_Rev_Apple = GL_UNSIGNED_SHORT_8_8_REV_APPLE,
    UInt16_8_8_Mesa      = GL_UNSIGNED_SHORT_8_8_MESA,
    UInt16_8_8_Rev_Mesa  = GL_UNSIGNED_SHORT_8_8_REV_MESA
};

enum TextureFormat { // internal format
    NoTextureFormat        = GL_NONE,
    R8_UNorm               = GL_R8,
    RG8_UNorm              = GL_RG8,
    RGB8_UNorm             = GL_RGB8,
    RGBA8_UNorm            = GL_RGBA8,
    R16_UNorm              = GL_R16,
    RG16_UNorm             = GL_RG16,
    RGB16_UNorm            = GL_RGB16,
    RGBA16_UNorm           = GL_RGBA16,
    Luminance8_UNorm       = GL_LUMINANCE8,
    Luminance16_UNorm      = GL_LUMINANCE16,
    LuminanceAlpha8_UNorm  = GL_LUMINANCE8_ALPHA8,
    LuminanceAlpha16_UNorm = GL_LUMINANCE16_ALPHA16,
    YCbCr_UNorm_Mesa       = GL_YCBCR_MESA,
    R16F                   = GL_R16F,
    RG16F                  = GL_RG16F,
    RGB16F                 = GL_RGB16F,
    RGBA16F                = GL_RGBA16F,
    R32F                   = GL_R32F,
    RG32F                  = GL_RG32F,
    RGB32F                 = GL_RGB32F,
    RGBA32F                = GL_RGBA32F,
};
enum TransferFormat {
    NoTransferFormat = GL_NONE,
    Red              = GL_RED,
    RG               = GL_RG,
    RGB              = GL_RGB,
    BGR              = GL_BGR,
    RGBA             = GL_RGBA,
    BGRA             = GL_BGRA,
    Luminance        = GL_LUMINANCE,
    LuminanceAlpha   = GL_LUMINANCE_ALPHA,
    YCbCr_422_Apple  = GL_YCBCR_422_APPLE,
    YCbCr_Mesa       = GL_YCBCR_MESA,
    OneComponent,
    TwoComponents
};

enum Filter {
    Nearest = GL_NEAREST,
    Linear  = GL_LINEAR,
};

enum WrapMode {
    Repeat         = GL_REPEAT,
    MirroredRepeat = GL_MIRRORED_REPEAT,
    Clamp          = GL_CLAMP,
    ClampToEdge    = GL_CLAMP_TO_EDGE,
    ClampToBorder  = GL_CLAMP_TO_BORDER
};

enum WrapDirection {
    DirectionS = GL_TEXTURE_WRAP_S,
    DirectionT = GL_TEXTURE_WRAP_T,
    DirectionR = GL_TEXTURE_WRAP_R
};

struct TransferInfo {
    TransferInfo() {}
    TransferInfo(TransferFormat format, TransferType type)
        : format(format), type(type) { }
    auto operator == (const TransferInfo &rhs) const -> bool
        { return format == rhs.format && type == rhs.type; }
    auto operator != (const TransferInfo &rhs) const -> bool
        { return !operator == (rhs); }
    TransferFormat format = OGL::BGRA;
    TransferType type = OGL::UInt32_8_8_8_8_Rev;
};

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

auto initialize(QOpenGLContext *ctx, bool debug) -> void;

auto finalize(QOpenGLContext *ctx) -> void;

auto errorString(GLenum error) -> const char*;

auto logError(const QByteArray &at) -> int;

auto hasExtension(Extension ext) -> bool;

auto maximumTextureSize() -> int;

SIA rg(const char *rg) -> QByteArray
{
    if (hasExtension(TextureRG))
        return QByteArray(rg);
    return QByteArray(rg).replace('g', 'a');
}

SIA func() -> QOpenGLFunctions* {
    auto ctx = QOpenGLContext::currentContext();
    return ctx ? ctx->functions() : nullptr;
}

auto availableFrambebufferFormats() -> QVector<TextureFormat>;
auto isSupportedFrambufferFormat(TextureFormat format) -> bool;
auto is16bitFramebufferFormatSupported() -> bool;

}

#define ENUM_CASE(e) case e: return #e##_b
SIA _ToLog(OGL::TransferType type) -> QByteArray {
    switch (type) {
    ENUM_CASE(OGL::UInt8);
    ENUM_CASE(OGL::UInt16);
    ENUM_CASE(OGL::Float32);
    ENUM_CASE(OGL::UInt32_8_8_8_8);
    ENUM_CASE(OGL::UInt32_8_8_8_8_Rev);
#ifdef Q_OS_MAC
    ENUM_CASE(OGL::UInt16_8_8_Apple);
    ENUM_CASE(OGL::UInt16_8_8_Rev_Apple);
#else
    ENUM_CASE(OGL::UInt16_8_8_Mesa);
    ENUM_CASE(OGL::UInt16_8_8_Rev_Mesa);
#endif
    ENUM_CASE(OGL::NoTransferType);
    }
    return QByteArray("OGL::InvalidTransferType");
}

SIA _ToLog(OGL::TextureFormat format) -> QByteArray {
    switch (format) {
    ENUM_CASE(OGL::R8_UNorm);
    ENUM_CASE(OGL::RG8_UNorm);
    ENUM_CASE(OGL::RGB8_UNorm);
    ENUM_CASE(OGL::RGBA8_UNorm);
    ENUM_CASE(OGL::R16_UNorm);
    ENUM_CASE(OGL::RG16_UNorm);
    ENUM_CASE(OGL::RGB16_UNorm);
    ENUM_CASE(OGL::RGBA16_UNorm);
    ENUM_CASE(OGL::Luminance8_UNorm);
    ENUM_CASE(OGL::Luminance16_UNorm);
    ENUM_CASE(OGL::LuminanceAlpha8_UNorm);
    ENUM_CASE(OGL::LuminanceAlpha16_UNorm);
    ENUM_CASE(OGL::YCbCr_UNorm_Mesa);
    ENUM_CASE(OGL::R16F);
    ENUM_CASE(OGL::RG16F);
    ENUM_CASE(OGL::RGB16F);
    ENUM_CASE(OGL::RGBA16F);
    ENUM_CASE(OGL::R32F);
    ENUM_CASE(OGL::RG32F);
    ENUM_CASE(OGL::RGB32F);
    ENUM_CASE(OGL::RGBA32F);
    ENUM_CASE(OGL::NoTextureFormat);
    }
    return QByteArray("OGL::InvalidTextureFormat");
}

SIA _ToLog(OGL::TransferFormat format) -> QByteArray {
    switch (format) {
    ENUM_CASE(OGL::Red);
    ENUM_CASE(OGL::RG);
    ENUM_CASE(OGL::RGB);
    ENUM_CASE(OGL::BGR);
    ENUM_CASE(OGL::RGBA);
    ENUM_CASE(OGL::BGRA);
    ENUM_CASE(OGL::Luminance);
    ENUM_CASE(OGL::LuminanceAlpha);
    ENUM_CASE(OGL::YCbCr_422_Apple);
    ENUM_CASE(OGL::YCbCr_Mesa);
    ENUM_CASE(OGL::OneComponent);
    ENUM_CASE(OGL::TwoComponents);
    ENUM_CASE(OGL::NoTransferFormat);
    }
    return QByteArray("OGL::InvalidTransferFormat");
}
#undef ENUM_CASE

#endif // OPENGLMISC_HPP
