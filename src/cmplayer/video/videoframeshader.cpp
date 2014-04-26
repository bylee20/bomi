#include "videoframeshader.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "log.hpp"
#include <tuple>

using Vertex = OGL::TextureVertex;

DECLARE_LOG_CONTEXT(VideoFrameShader)

static const QByteArray shaderTemplate(
#include "videoframeshader.glsl.hpp"
);

VideoFrameShader::VideoFrameShader()
    : m_vbo(QOpenGLBuffer::VertexBuffer)
{
    auto ctx = QOpenGLContext::currentContext();
    m_dma = ctx->hasExtension("GL_APPLE_client_storage") && ctx->hasExtension("GL_APPLE_texture_range");
    if (m_dma)
        _Info("Direct memoery access(DMA) is available.");
    m_lutInt[0].create();
    m_lutInt[1].create();
    m_vbo.create();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
}

VideoFrameShader::~VideoFrameShader() {
    release();
    m_vbo.destroy();
}

void VideoFrameShader::release() {
    m_direct = false;
    if (m_textures.isEmpty())
        return;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    if (m_mixer) {
        m_textures[0].bind();
        _Delete(m_mixer);
    }
    for (auto &texture : m_textures)
        texture.destroy();
    m_textures.clear();
}

void VideoFrameShader::updateTexCoords() {
    QPointF p1 = {0.0, 0.0}, p2 = {1.0, 1.0};
    const auto &format = m_frame.format();
    if (m_target == OGL::Target2D)
        p2.rx() = _Ratio(format.width(), format.alignedWidth());
    else
        p2 = QPointF(format.width(), format.height());
    if (m_frame.isFlipped())
        qSwap(p1.ry(), p2.ry());
    m_texRect = {p1, p2};

    m_vbo.bind();
    m_vbo.allocate(4*sizeof(Vertex));
    auto v = static_cast<Vertex*>(m_vbo.map(QOpenGLBuffer::WriteOnly));
    Vertex::fillAsTriangleStrip(v, {-1, -1}, {1, 1}, p1, p2);
    m_vbo.unmap();
    m_vbo.release();
}

void VideoFrameShader::setColor(const VideoColor &color) {
    m_color = color;
    updateColorMatrix();
}

void VideoFrameShader::setRange(ColorRange range) {
    m_range = range;
    updateColorMatrix();
}

void VideoFrameShader::setChromaInterpolator(InterpolatorType type) {
    for (auto &shader : m_shaders) {
        if (shader.interpolator->type() != type) {
            shader.interpolator = Interpolator::get(type);
            shader.rebuild = true;
        }
    }
}

void VideoFrameShader::updateColorMatrix() {
    auto color = m_color;
    if (m_effects & VideoRendererItem::Grayscale)
        color.setSaturation(-100);
    auto range = m_range;
    const bool pc = m_frame.format().range() == MP_CSP_LEVELS_PC;
    switch (range) {
    case ColorRange::Auto:
        range = pc ? ColorRange::Full : ColorRange::Limited;
        break;
    case ColorRange::Remap:
    case ColorRange::Extended:
        if (pc)
            range = ColorRange::Full;
        break;
    default:
        break;
    }
    color.matrix(m_mul_mat, m_add_vec, m_csp, range);
    if (m_effects & VideoRendererItem::InvertColor) {
        m_mul_mat *= -1;
        m_add_vec = QVector3D(1, 1, 1) - m_add_vec;
    }
    m_defaultColor = m_color.isZero();
}

void VideoFrameShader::setEffects(int effects) {
    if (m_effects == effects)
        return;
    const int old = m_effects;
    m_effects = effects;
    auto isColorEffect = [] (int e) -> bool { return e & ColorEffects; };
    if (isColorEffect(old) != isColorEffect(m_effects))
        updateColorMatrix();
    for (auto &shader : m_shaders) {
        if (_Change(shader.kernel, hasKernelEffects()))
            shader.rebuild = true;
    }
}

void VideoFrameShader::setDeintMethod(DeintMethod method) {
    m_deint = method;
}

void VideoFrameShader::updateShader() {
    Q_ASSERT(!m_frame.format().isEmpty());
    int deint = 0;
    auto isX11HwAcc = [] (const VideoFrame &frame) {
        return frame.format().imgfmt() == IMGFMT_VAAPI
               || frame.format().imgfmt() == IMGFMT_VDPAU;
    };
    if (m_frame.isInterlaced() && !isX11HwAcc(m_frame)) {
        switch (m_deint) {
        case DeintMethod::Bob:
            deint = 1;
            break;
        case DeintMethod::LinearBob:
            deint = 2;
            break;
        default:
            break;
        }
    }
    auto &shader = m_shaders[deint];
    if (shader.rebuild) {
        shader.rebuild = false;
        const VideoFormat &format = m_frame.format();
        auto &prog = shader.program;
        prog.removeAllShaders();
        QByteArray header;
        header += "#define TEX_COUNT " + QByteArray::number(m_textures.size()) + "\n";
        header += "const float texWidth = " + _N((double)format.alignedWidth(), 1).toLatin1() + ";\n";
        header += "const float texHeight = " + _N((double)format.alignedHeight(), 1).toLatin1() + ";\n";
        auto declareVec2 = [] (const QString &name, const QPointF &p) -> QString {
            return _L("const vec2 ") % name % _L(" = vec2(") % _N(p.x(), 6) % _L(", ") % _N(p.y(), 6) % _L(");\n");
        };
        auto cc2string = [declareVec2, this] (int i) -> QString {
            QPointF cc = {1.0, 1.0};
            if (i < m_textures.size())
                cc = m_textures[i].correction();
            return declareVec2("cc" + _N(i), cc);
        };
        header += cc2string(1).toLatin1();
        header += cc2string(2).toLatin1();
        const double chroma_x = m_frame.format().chroma() == MP_CHROMA_LEFT ? -0.5 : 0.0;
        header += declareVec2("chroma_location", {chroma_x, 0.0});
        if (m_target != OGL::Target2D || format.isEmpty())
            header += "#define USE_RECTANGLE\n";
        if (hasKernelEffects())
            header += "#define USE_KERNEL3x3\n";

        header += "#define USE_DEINT " + QByteArray::number(deint) + "\n";
        header += R"(
#ifdef USE_RECTANGLE
const vec4 dxdy = vec4(1.0, 1.0, -1.0, 0.0);
const vec2 chroma_offset = chroma_location;
#define sampler2Dg sampler2DRect
#define texture2Dg texture2DRect
#else
const vec4 dxdy = vec4(1.0/texWidth, 1.0/texHeight, -1.0/texWidth, 0.0);
const vec2 chroma_offset = chroma_location*dxdy.xy;
#define sampler2Dg sampler2D
#define texture2Dg texture2D
#endif
const vec2 dxy = dxdy.xy;
const vec2 tex_size = vec2(texWidth, texHeight);
)";
        auto interpolator = m_csp != MP_CSP_RGB ? shader.interpolator
            : Interpolator::get(InterpolatorType::Bilinear);
        m_lutCount = interpolator->textures();
        Q_ASSERT(0 <= m_lutCount && m_lutCount < 3);

        interpolator->allocate(&m_lutInt[0], &m_lutInt[1]);
        auto common = interpolator->shader() + shaderTemplate;
        auto fragCode = header;
        fragCode += "#define FRAGMENT\n";
        fragCode += common;
        fragCode += m_texel;

        auto vertexCode = header;
        vertexCode += "#define VERTEX\n";
        vertexCode += common;

        prog.addShaderFromSourceCode(QOpenGLShader::Fragment, fragCode);
        prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexCode);

        prog.bindAttributeLocation("vPosition", AttrPosition);
        prog.bindAttributeLocation("vCoord", AttrTexCoord);

        prog.link();
        m_prog = nullptr;
    }
    if (_Change(m_prog, &shader.program)) {
        loc_tex[0] = m_prog->uniformLocation("tex0");
        loc_tex[1] = m_prog->uniformLocation("tex1");
        loc_tex[2] = m_prog->uniformLocation("tex2");
        loc_top_field = m_prog->uniformLocation("top_field");
        loc_mul_mat = m_prog->uniformLocation("mul_mat");
        loc_add_vec = m_prog->uniformLocation("add_vec");
        loc_cc[0] = m_prog->uniformLocation("cc0");
        loc_cc[1] = m_prog->uniformLocation("cc1");
        loc_cc[2] = m_prog->uniformLocation("cc2");
        loc_vMatrix = m_prog->uniformLocation("vMatrix");
        loc_kern_c = m_prog->uniformLocation("kern_c");
        loc_kern_d = m_prog->uniformLocation("kern_d");
        loc_kern_n = m_prog->uniformLocation("kern_n");
        for (int i=0; i<m_lutCount; ++i) {
            auto name = QByteArray("lut_int") + QByteArray::number(i+1);
            loc_lut_int[i] = m_prog->uniformLocation(name);
        }
    }
}

void VideoFrameShader::reupload() {
    updateShader();
    upload();
}

void VideoFrameShader::upload() {
    if (m_textures.isEmpty())
        return;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    if (m_mixer) {
#ifndef Q_OS_MAC
        m_textures[0].bind();
#endif
        m_mixer->upload(m_frame, m_deint != DeintMethod::None);
    } else if (!m_frame.isAdditional()) {
        if (m_dma)
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        for (int i=0; i<m_textures.size(); ++i) {
            m_textures[i].bind();
            m_textures[i].upload(m_frame.data(m_textures[i].plane()));
        }
        if (m_dma)
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
    }
}

bool VideoFrameShader::upload(VideoFrame &frame) {
    bool changed = false;
    if (!frame.isAdditional()) {
        changed = m_frame.format() != frame.format();
        if (m_dma && frame.format().imgfmt() != IMGFMT_VDA) {
            if (changed)
                m_frame.allocate(frame.format());
            m_frame.doDeepCopy(frame);
        } else
            m_frame.swap(frame);
        if (changed) {
            fillInfo();
            updateTexCoords();
            updateColorMatrix();
        }
        updateShader();
        if (m_textures.isEmpty())
            return changed;
    } else
        m_frame.copyInterlaced(frame);
    upload();
    return changed;
}

void VideoFrameShader::render(const Kernel3x3 &k3x3) {
    if (!m_prog || m_textures.isEmpty())
        return;
    glViewport(0, 0, m_frame.format().width(), m_frame.format().height());
    auto f = QOpenGLContext::currentContext()->functions();
    f->glActiveTexture(GL_TEXTURE0);
    OpenGLTextureBinder<OGL::Target1D> binder1;
    OpenGLTextureBaseBinder binder2(m_target, m_binding);

    m_prog->bind();
    m_prog->setUniformValue(loc_top_field, (float)m_frame.isTopField());
    m_prog->setUniformValue(loc_add_vec, m_add_vec);
    m_prog->setUniformValue(loc_mul_mat, m_mul_mat);
    if (hasKernelEffects()) {
        m_prog->setUniformValue(loc_kern_c, k3x3.center());
        m_prog->setUniformValue(loc_kern_n, k3x3.neighbor());
        m_prog->setUniformValue(loc_kern_d, k3x3.diagonal());
    }

    auto texPos = 0;
    for (int i=0; i<m_textures.size(); ++i, ++texPos) {
        m_textures[i].bind(m_prog, loc_tex[i], texPos);
        m_prog->setUniformValue(loc_cc[i], m_textures[i].correction());
    }
    for (int i=0; i<m_lutCount; ++i, ++texPos)
        m_lutInt[i].bind(m_prog, loc_lut_int[i], texPos);

    m_vbo.bind();
    SET_ATTR_COORD(m_prog, AttrPosition, Vertex, position);
    SET_ATTR_COORD(m_prog, AttrTexCoord, Vertex, texCoord);
    m_prog->enableAttributeArray(AttrPosition);
    m_prog->enableAttributeArray(AttrTexCoord);

    f->glActiveTexture(GL_TEXTURE0);
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_BLEND);

    m_prog->disableAttributeArray(AttrTexCoord);
    m_prog->disableAttributeArray(AttrPosition);
    m_prog->release();
    m_vbo.release();
}


void VideoFrameShader::fillInfo() {
    release();
    const auto &format = m_frame.format();
    m_shaders[0].rebuild = m_shaders[1].rebuild = true;

    m_csp = format.colorspace();
    std::tie(m_target, m_binding) = !m_dma && format.imgfmt() != IMGFMT_VDA
        ? std::forward_as_tuple(OGL::Target2D, OGL::Binding2D)
        : std::forward_as_tuple(OGL::TargetRectangle, OGL::BindingRectangle);
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    auto cc = [this, &format] (int factor, double rect) {
        for (int i=1; i<m_textures.size(); ++i) {
            if (format.imgfmt() != IMGFMT_VDA && i < format.planes())
                m_textures[i].correction().rx() = (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*factor);
            if (m_target == OGL::TargetRectangle)
                m_textures[i].correction() *= rect;
        }
    };

    auto addCustom = [this, &format] (int plane, int width, int height, const OpenGLTextureTransferInfo &fmt) {
        OpenGLTexture2D texture(m_target);
        texture.setAttributes(width, height, fmt);
        texture.plane() = plane;
        texture.create();
        if (format.imgfmt() != IMGFMT_VDA) {
            if (m_dma) {
                glEnable(texture.target());
                texture.bind();
                glTexParameteri(texture.target(), GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);
                glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
                texture.initialize(m_frame.data(plane));
                glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
            } else {
                texture.bind();
                texture.initialize();
            }
        }
        m_textures.append(texture);
    };

    auto add = [this, addCustom, &format] (int plane, OGL::TransferFormat fmt) {
        int components = 4;
        if (fmt == OGL::OneComponent)
            components = 1;
        else if (fmt == OGL::TwoComponents)
            components = 2;
        addCustom(plane, format.bytesPerLine(plane)/components, format.lines(plane), OpenGLCompat::textureTransferInfo(fmt));
    };

    const int bits = format.encodedBits();
    const bool little = format.isLittleEndian();
    m_texel = "vec3 texel(const in vec4 tex0) {return tex0.rgb;}"; // passthrough
    switch (format.type()) {
    case IMGFMT_444P:
        add(0, OGL::OneComponent); add(1, OGL::OneComponent); add(2, OGL::OneComponent);
        m_texel = R"(
            vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
                return vec3(tex0.r, tex1.r, tex2.r);
            }
        )";
        break;
    case IMGFMT_420P:
        add(0, OGL::OneComponent); add(1, OGL::OneComponent); add(2, OGL::OneComponent); cc(2, 0.5);
        m_texel = R"(
            vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
                return vec3(tex0.r, tex1.r, tex2.r);
            }
        )";
        break;
    case IMGFMT_420P16_BE:
    case IMGFMT_420P14_BE:
    case IMGFMT_420P12_BE:
    case IMGFMT_420P10_BE:
    case IMGFMT_420P9_BE:
    case IMGFMT_420P16_LE:
    case IMGFMT_420P14_LE:
    case IMGFMT_420P12_LE:
    case IMGFMT_420P10_LE:
    case IMGFMT_420P9_LE:
        m_texel = R"(
            float convBits(const in vec4 tex) {
                const vec2 c = vec2(256.0, 1.0)/(256.0*??.0/255.0 + 1.0);
                return dot(tex.!!, c);
            }
            vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
                return vec3(convBits(tex0), convBits(tex1), convBits(tex2));
            }
        )";
        m_texel.replace("??", QByteArray::number((1 << (bits-8))-1));
        m_texel.replace("!!", OpenGLCompat::rg(little ? "gr" : "rg"));
        add(0, OGL::TwoComponents); add(1, OGL::TwoComponents); add(2, OGL::TwoComponents); cc(2, 0.5);
        break;
    case IMGFMT_NV12:
    case IMGFMT_NV21:
        m_texel = R"(
            vec3 texel(const in vec4 tex0, const in vec4 tex1) {
                return vec3(tex0.r, tex1.!!);
            }
        )";
        m_texel.replace("!!", OpenGLCompat::rg(format.type() == IMGFMT_NV12 ? "rg" : "gr"));
        add(0, OGL::OneComponent); add(1, OGL::TwoComponents); cc(1, 0.5);
        break;
    case IMGFMT_YUYV:
    case IMGFMT_UYVY:
        if (OpenGLCompat::hasExtension(OpenGLCompat::AppleYCbCr422)) {
            _Debug("Use GL_APPLE_ycbcr_422.");
            OpenGLTextureTransferInfo info;
            info.texture = OGL::RGB8_UNorm;
            info.transfer.format = OGL::YCbCr_422_Apple;
            info.transfer.type = format.type() == IMGFMT_YUYV ? OGL::UInt16_8_8_Rev_Apple : OGL::UInt16_8_8_Apple;
            addCustom(0, format.width(), format.height(), info);
            m_csp = MP_CSP_RGB;
        } else if (OpenGLCompat::hasExtension(OpenGLCompat::MesaYCbCrTexture)) {
            _Debug("Use GL_MESA_ycbcr_texture.");
            m_texel = R"(vec3 texel(const in int coord) { return texture0(coord).g!!; })";
            m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "br" : "rb");
            OpenGLTextureTransferInfo info;
            info.texture = OGL::YCbCr_UNorm_Mesa;
            info.transfer.format = OGL::YCbCr_Mesa;
            info.transfer.type = format.type() == IMGFMT_YUYV ? OGL::UInt16_8_8_Rev_Mesa : OGL::UInt16_8_8_Mesa;
            addCustom(0, format.width(), format.height(), info);
        } else {
            m_texel = R"(
                vec3 texel(const in vec4 tex0, const in vec4 tex1) {
                    return vec3(tex0.?, tex1.!!);
                }
            )";
            m_texel.replace("?", format.type() == IMGFMT_YUYV ? "r" : OpenGLCompat::rg("g"));
            m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "ga" : "br");
            add(0, OGL::TwoComponents); add(0, OGL::BGRA);
            if (m_target == OGL::TargetRectangle)
                m_textures[1].correction().rx() *= 0.5;
        }
        break;
    case IMGFMT_BGRA:
    case IMGFMT_BGR0:
        add(0, OGL::BGRA);
        m_direct = true;
        break;
    case IMGFMT_RGBA:
    case IMGFMT_RGB0:
        add(0, OGL::RGBA);
        m_direct = true;
        break;
    case IMGFMT_ABGR:
    case IMGFMT_0BGR:
        add(0, OGL::BGRA);
        m_texel.replace(".rgb", ".arg");
        break;
    case IMGFMT_ARGB:
    case IMGFMT_0RGB:
        add(0, OGL::BGRA);
        m_texel.replace(".rgb", ".gra");
        break;
    default:
        break;
    }
    m_mixer = HwAcc::createMixer(m_textures, format);
    if (m_mixer)
        m_direct = m_mixer->directRendering();
}
