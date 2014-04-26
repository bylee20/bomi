#ifndef VIDEOFRAMESHADER_HPP
#define VIDEOFRAMESHADER_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"
#include "videocolor.hpp"
#include "videorendereritem.hpp"
#include "deintinfo.hpp"
#include "videoframe.hpp"
#include "opengl/openglcompat.hpp"
#include "opengl/interpolator.hpp"
#include "opengl/openglvertex.hpp"

class HwAccMixer;

class VideoFrameShader {
    enum Attr {AttrPosition, AttrTexCoord};
    static constexpr auto KernelEffects = VideoRendererItem::KernelEffects;
    static constexpr auto ColorEffects = VideoRendererItem::ColorEffects;
    static constexpr auto ShaderEffects = VideoRendererItem::ShaderEffects;
public:
    VideoFrameShader();
    VideoFrameShader(const VideoFrameShader &) = delete;
    VideoFrameShader &operator = (const VideoFrameShader &) = delete;
    ~VideoFrameShader();
    void render(const Kernel3x3 &k3x3);
    GLenum target() const { return m_target; }
    void setDeintMethod(DeintMethod method);
    void setEffects(int effects);
    void setColor(const VideoColor &color);
    void setRange(ColorRange range);
    bool upload(VideoFrame &frame);
    void getCoords(double &x1, double &y1, double &x2, double &y2)
    { m_texRect.getCoords(&x1, &y1, &x2, &y2); }
    void setChromaInterpolator(InterpolatorType type);
    bool isDirectlyRenderable() const
    { return m_direct && m_defaultColor && !(m_effects & ShaderEffects); }
    const OpenGLTexture2D &renderTarget() const { return m_textures[0]; }
    void reupload();
private:
    void upload();
    void release();
    void updateColorMatrix();
    bool hasKernelEffects() const { return KernelEffects & m_effects; }
    void fillInfo();
    void updateTexCoords();
private:
    struct ShaderInfo {
        QOpenGLShaderProgram program;
        bool rebuild = true, kernel = false;
        const Interpolator *interpolator
            = Interpolator::get(InterpolatorType::Bilinear);
    };
    void updateShader();
    VideoFrame m_frame;
    ShaderInfo m_shaders[3];
    QOpenGLShaderProgram *m_prog = nullptr;
    VideoColor m_color;
    mp_csp m_csp;
    ColorRange m_range = ColorRange::Auto;
    OGL::Target m_target = OGL::Target2D;
    OGL::Binding m_binding = OGL::Binding2D;
    QMatrix3x3 m_mul_mat;
    QVector3D m_sub_vec, m_add_vec;
    int m_effects = 0, m_lutCount = 0;
    DeintMethod m_deint = DeintMethod::None;
    QList<OpenGLTexture2D> m_textures;
    QByteArray m_texel;
    bool m_dma = false, m_check = true, m_direct = false, m_defaultColor = true;
    QRectF m_texRect;
    QPointF m_chroma = {0.0, 0.0};
    OpenGLTexture1D m_lutInt[2];
    HwAccMixer *m_mixer = nullptr;
    QOpenGLBuffer m_vbo;
    int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field;
    int loc_add_vec, loc_mul_mat, loc_vMatrix;
    int loc_tex[3] = {-1, -1, -1}, loc_cc[3] = {-1, -1, -1};
    int loc_lut_int[2] = {-1, -1};
};

#endif // VIDEOFRAMESHADER_HPP
