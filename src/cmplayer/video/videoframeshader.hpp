#ifndef VIDEOFRAMESHADER_HPP
#define VIDEOFRAMESHADER_HPP

#include "videocolor.hpp"
#include "opengl/opengltexture1d.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"
#include "enum/deintmethod.hpp"
#include "enum/videoeffect.hpp"
#include "videotexture.hpp"
#include "mpimage.hpp"

class HwAccMixer;                       class Kernel3x3;
class Interpolator;
enum class InterpolatorType;

class VideoFrameShader {
    enum Attr {AttrPosition, AttrTexCoord};
public:
    VideoFrameShader();
    VideoFrameShader(const VideoFrameShader &) = delete;
    VideoFrameShader &operator = (const VideoFrameShader &) = delete;
    ~VideoFrameShader();
    auto render(const Kernel3x3 &k3x3) -> void;
    auto target() const -> OGL::Target { return m_target; }
    auto setDeintMethod(DeintMethod method) -> void;
    auto setEffects(VideoEffects effects) -> void;
    auto setColor(const VideoColor &color,
                  ColorSpace space, ColorRange range) -> void;
    auto setChromaUpscaler(InterpolatorType type) -> void;
    auto isDirectlyRenderable() const -> bool;
    auto upload(const MpImage &mpi) -> void;
    auto upload(const MpImage &mpi, VideoTexture &texture) -> void;
    auto prepare(const MpImage &mpi) -> void;
    auto setFormat(const mp_image_params &params, bool inverted) -> void;
    auto directlyRenderableTexture() -> const VideoTexture&
        { return *m_textures.front(); }
    auto useDMA() const -> bool { return m_dma && m_params.imgfmt != IMGFMT_VDA; }
private:
    auto release() -> void;
    auto updateColorMatrix() -> void;
    auto hasKernelEffects() const -> bool { return KernelEffects & m_effects; }
    auto fillInfo(const MpImage &mpi) -> void;
    auto updateTexCoords() -> void;
private:
    struct ShaderInfo {
        ShaderInfo();
        QOpenGLShaderProgram program;
        QOpenGLShader vertexShader{QOpenGLShader::Vertex};
        QOpenGLShader fragmentShader{QOpenGLShader::Fragment};
        bool rebuild = true, kernel = false;
        const Interpolator *interpolator = nullptr;
    };
    auto updateShader(int deint) -> void;
    bool m_refill = false;
    QVector<QSize> m_bytes;
    QVector<VideoTexture*> m_textures;
    QVector<OpenGLTextureTransferInfo> m_transferInfos;
    mp_imgfmt m_imgfmtOut = IMGFMT_NONE;
//    mp_csp m_cspOut = MP_CSP_BT_709;
    ShaderInfo m_shaders[3];
    QOpenGLShaderProgram *m_prog = nullptr;
    VideoColor m_color;
    mp_image_params m_params;
    ColorSpace m_space = ColorSpace::Auto;
    ColorRange m_range = ColorRange::Auto;
    OGL::Target m_target = OGL::Target2D;
    OGL::Binding m_binding = OGL::Binding2D;
    QMatrix4x4 m_mul_mat;
    int m_lutCount = 0;
    VideoEffects m_effects = 0;
    DeintMethod m_deint = DeintMethod::None;
    QByteArray m_texel;
    bool m_dma = false, m_direct = false, m_defaultColor = true;
    bool m_top = false, m_inverted = false, m_additional = false;
    bool m_hwdec = false;
    QPointF m_chroma = {0.0, 0.0};
    OpenGLTexture1D m_lutInt[2];
    QOpenGLBuffer m_vbo;
    int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field;
    int loc_mul_mat, loc_vMatrix;
    int loc_tex[3] = {-1, -1, -1}, loc_cc[3] = {-1, -1, -1};
    int loc_lut_int[2] = {-1, -1};
};

inline auto VideoFrameShader::isDirectlyRenderable() const -> bool
{
    return m_direct && m_defaultColor && !(m_effects & ShaderEffects);
}

#endif // VIDEOFRAMESHADER_HPP
