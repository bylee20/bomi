#ifndef VIDEOFRAMESHADER_HPP
#define VIDEOFRAMESHADER_HPP

#include "stdafx.hpp"
#include "opengl/openglcompat.hpp"
#include "opengl/interpolator.hpp"
#include "videoformat.hpp"
#include "videocolor.hpp"
#include "videorendereritem.hpp"
#include "deintinfo.hpp"
#include "videoframe.hpp"

class HwAccMixer;

class VideoFrameShader {
    enum {vPosition, vCoord};
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
    void getCoords(double &x1, double &y1, double &x2, double &y2) {
        x1 = m_coords.left(); y1 = m_coords.top();
        x2 = m_coords.right(); y2 = m_coords.bottom();
    }
    void setChromaInterpolator(InterpolatorType type);
    bool directRendering() const { return m_direct && m_defaultColor && !(m_effects & VideoRendererItem::ShaderEffects); }
    const OpenGLTexture2D &renderTarget() const { return m_textures[0]; }
    void reupload();
private:
    void upload();
    void release();
    void updateColorMatrix();
    static bool isKernelEffect(int effect) { return VideoRendererItem::KernelEffects & effect; }
    static bool isColorEffect(int effect) { return VideoRendererItem::ColorEffects & effect; }
    bool hasKernelEffects() const { return isKernelEffect(m_effects); }
    void fillInfo();
    void fillArrays();
    static QVector<GLfloat> makeArray(const QPointF &p1, const QPointF &p2) {
        QVector<GLfloat> vec(4*2);
        auto p = vec.data();
        *p++ = p1.x(); *p++ = p1.y();
        *p++ = p1.x(); *p++ = p2.y();
        *p++ = p2.x(); *p++ = p1.y();
        *p++ = p2.x(); *p++ = p2.y();
        return vec;
    }
    void updateTexCoords();
private:
    struct ShaderInfo {
        QOpenGLShaderProgram program;
        bool rebuild = true, kernel = false;
        const Interpolator *interpolator = Interpolator::get(InterpolatorType::Bilinear);
    };
    void updateShader();
    bool isX11HwAcc(const VideoFrame &frame) {
        return frame.format().imgfmt() == IMGFMT_VAAPI || frame.format().imgfmt() == IMGFMT_VDPAU;
    }
    VideoFrame m_frame;
    ShaderInfo m_shaders[3];
    QOpenGLShaderProgram *m_prog = nullptr;
    VideoColor m_color;
    mp_csp m_csp; ColorRange m_range = ColorRange::Auto;
    OGL::Target m_target = OGL::Target2D;
    OGL::Binding m_binding = OGL::Binding2D;
    QMatrix3x3 m_mul_mat;
    QMatrix4x4 m_vMatrix;
    QVector3D m_sub_vec, m_add_vec;
    QVector<GLfloat> m_vCoords, m_vPositions;
    int m_effects = 0;
    DeintMethod m_deint = DeintMethod::None;
    int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field;
    int loc_add_vec, loc_mul_mat, loc_vMatrix;
    int loc_tex[3] = {-1, -1, -1}, loc_cc[3] = {-1, -1, -1};
    int loc_lut_int[2] = {-1, -1};
    int m_lutCount = 0;
    QList<OpenGLTexture2D> m_textures;
    QByteArray m_texel;
    bool m_dma = false, m_check = true, m_direct = false, m_defaultColor = true;
    QRectF m_coords, m_positions;
    QPointF m_chroma = {0.0, 0.0};
    OpenGLTexture1D m_lutInt[2];
    HwAccMixer *m_mixer = nullptr;
};

#endif // VIDEOFRAMESHADER_HPP
