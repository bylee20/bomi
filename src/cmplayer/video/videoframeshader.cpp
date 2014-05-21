#include "videoframeshader.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "kernel3x3.hpp"
#include "misc/log.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "enum/interpolatortype.hpp"
#include <tuple>

VideoFrameShader::ShaderInfo::ShaderInfo()
{
    interpolator = Interpolator::get(InterpolatorType::Bilinear);
}

using Vertex = OGL::TextureVertex;

DECLARE_LOG_CONTEXT(VideoFrameShader)

static const QByteArray shaderTemplate(
#include "videoframeshader.glsl.hpp"
);

VideoFrameShader::VideoFrameShader()
    : m_vbo(QOpenGLBuffer::VertexBuffer)
{
    auto ctx = QOpenGLContext::currentContext();
    m_dma = ctx->hasExtension("GL_APPLE_client_storage")
            && ctx->hasExtension("GL_APPLE_texture_range");
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

auto VideoFrameShader::release() -> void
{
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

auto VideoFrameShader::setFlipped(bool flipped) -> void
{
    if (_Change(m_flipped, flipped))
        updateTexCoords();
}

auto VideoFrameShader::updateTexCoords() -> void
{
    Q_ASSERT(!m_textures.isEmpty());

    QPointF p1 = {0.0, 0.0}, p2 = {1.0, 1.0};
    if (m_target == OGL::Target2D)
        p2.rx() = _Ratio(m_params.w, m_textures.first().width());
    else
        p2 = QPointF(m_params.w, m_params.h);
    if (m_flipped)
        std::swap(p1.ry(), p2.ry());

    m_vbo.bind();
    m_vbo.allocate(4*sizeof(Vertex));
    auto v = static_cast<Vertex*>(m_vbo.map(QOpenGLBuffer::WriteOnly));
    Vertex::fillAsTriangleStrip(v, {-1, -1}, {1, 1}, p1, p2);
    m_vbo.unmap();
    m_vbo.release();
}

auto VideoFrameShader::setColor(const VideoColor &color) -> void
{
    m_color = color;
    updateColorMatrix();
}

auto VideoFrameShader::setRange(ColorRange range) -> void
{
    m_range = range;
    updateColorMatrix();
}

auto VideoFrameShader::setChromaUpscaler(InterpolatorType type) -> void
{
    for (auto &shader : m_shaders) {
        if (shader.interpolator->type() != type) {
            shader.interpolator = Interpolator::get(type);
            shader.rebuild = true;
        }
    }
}

auto VideoFrameShader::updateColorMatrix() -> void
{
    auto color = m_color;
    if (m_effects & VideoEffect::Gray)
        color.setSaturation(-100);
    auto range = m_range;
    const bool pc = m_params.colorlevels == MP_CSP_LEVELS_PC;
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
    color.matrix(m_mul_mat, m_add_vec, m_cspOut, range);
    if (m_effects & VideoEffect::Invert) {
        m_mul_mat *= -1;
        m_add_vec = QVector3D(1, 1, 1) - m_add_vec;
    }
    m_defaultColor = m_color.isZero();
}

auto VideoFrameShader::setEffects(VideoEffects effects) -> void
{
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

auto VideoFrameShader::setDeintMethod(DeintMethod method) -> void
{
    m_deint = method;
}

auto VideoFrameShader::updateShader(int deint) -> void
{
    auto &shader = m_shaders[deint];
    if (shader.rebuild) {
        shader.rebuild = false;
        auto &prog = shader.program;
        prog.removeAllShaders();
        QByteArray header;
        header += "#define TEX_COUNT "
                  + QByteArray::number(m_textures.size()) + "\n";
        auto sizeNum = [] (double val) { return _N(val, 1).toLatin1(); };
        header += "const float texWidth = "
                  + sizeNum(m_textures.first().width()) + ";\n";
        header += "const float texHeight = "
                  + sizeNum(m_textures.first().height()) + ";\n";
        auto declareVec2 = [] (const QString &name,
                               const QPointF &p) -> QString {
            return _L("const vec2 ") % name % _L(" = vec2(")
                   % _N(p.x(), 6) % _L(", ") % _N(p.y(), 6) % _L(");\n");
        };
        auto cc2string = [declareVec2, this] (int i) -> QString {
            QPointF cc = {1.0, 1.0};
            if (i < m_textures.size())
                cc = m_textures[i].correction();
            return declareVec2("cc" + _N(i), cc);
        };
        header += cc2string(1).toLatin1();
        header += cc2string(2).toLatin1();
        const auto chromaLeft = m_params.chroma_location == MP_CHROMA_LEFT;
        const qreal chroma_x = chromaLeft ? -0.5 : 0.0;
        header += declareVec2("chroma_location", {chroma_x, 0.0});
        if (m_target != OGL::Target2D)
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
        auto interpolator = m_cspOut != MP_CSP_RGB ? shader.interpolator
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

        shader.fragmentShader.compileSourceCode(fragCode);
        shader.vertexShader.compileSourceCode(vertexCode);

        prog.addShader(&shader.fragmentShader);
        prog.addShader(&shader.vertexShader);

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

auto VideoFrameShader::setFormat(const VideoFormat &format) -> void
{
    m_params = format.params();
    m_imgfmtOut = m_params.imgfmt;
    m_refill = true;
}

auto VideoFrameShader::upload(const mp_image *mpi) -> void
{
    static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;
    if (!mpi)
        return;
    if (!m_refill) {
        for (int i = 0; i < mpi->fmt.num_planes; ++i) {
            if (mpi->stride[i] != m_bytes[i].width()) {
                m_refill = true;
                break;
            }
        }
    }
    if (m_refill) {
        fillInfo(mpi);
        updateTexCoords();
        updateColorMatrix();
        m_refill = false;
    }
    const bool additional = mpi->fields & MP_IMGFIELD_ADDITIONAL;
    if (!additional) {
        auto getCategory = [&] () {
            if (!(mpi->fields & (MP_IMGFIELD_TOP | MP_IMGFIELD_BOTTOM)))
                return 0;
            if (m_params.imgfmt != IMGFMT_VAAPI)
                return 0;
            if (m_params.imgfmt != IMGFMT_VDPAU)
                return 0;
            if (m_deint == DeintMethod::Bob)
                return 1;
            if (m_deint == DeintMethod::LinearBob)
                return 2;
            return 0;
        };
        updateShader(getCategory());
    }
    if (m_textures.isEmpty())
        return;
    m_top = mpi->fields & MP_IMGFIELD_TOP;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    if (m_mixer) {
#ifndef Q_OS_MAC
        m_textures[0].bind();
#endif
        m_mixer->upload(mpi, m_deint != DeintMethod::None);
    } else if (!additional) {
        if (m_dma) {
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
            for (int i=0; i<m_textures.size(); ++i) {
                m_textures[i].bind();
                m_textures[i].initialize(mpi->planes[m_textures[i].plane()]);
            }
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        } else {
            for (int i=0; i<m_textures.size(); ++i) {
                m_textures[i].bind();
                m_textures[i].upload(mpi->planes[m_textures[i].plane()]);
            }
        }
    }
}

auto VideoFrameShader::render(const Kernel3x3 &k3x3) -> void
{
    if (!m_prog || m_textures.isEmpty())
        return;
    glViewport(0, 0, m_params.w, m_params.h);
    auto f = QOpenGLContext::currentContext()->functions();
    f->glActiveTexture(GL_TEXTURE0);
    OpenGLTextureBinder<OGL::Target1D> binder1;
    OpenGLTextureBaseBinder binder2(m_target, m_binding);

    m_prog->bind();
    m_prog->setUniformValue(loc_top_field, (float)m_top);
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


auto VideoFrameShader::fillInfo(const mp_image *mpi) -> void
{
    release();

    m_bytes.resize(mpi->fmt.num_planes);
    for (int i = 0; i < m_bytes.size(); ++i) {
        m_bytes[i].rwidth() = mpi->stride[i];
        m_bytes[i].rheight() = mpi->h >> mpi->fmt.ys[i];
    }
    auto bytes = m_bytes;

    m_mixer = HwAcc::createMixer(m_params.imgfmt, {m_params.w, m_params.h});
    if (m_mixer)
        m_imgfmtOut = m_mixer->getAligned(mpi, &bytes);

    m_shaders[0].rebuild = m_shaders[1].rebuild = true;

    m_cspOut = m_params.colorspace;
    std::tie(m_target, m_binding) = !m_dma && m_params.imgfmt != IMGFMT_VDA
        ? std::forward_as_tuple(OGL::Target2D, OGL::Binding2D)
        : std::forward_as_tuple(OGL::TargetRectangle, OGL::BindingRectangle);
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    auto cc = [&] (int factor, double rect) {
        for (int i=1; i<m_textures.size(); ++i) {
            if (m_params.imgfmt != IMGFMT_VDA && i < bytes.size()) {
                const double l1 = bytes[0].width();
                const double l2 = bytes[i].width()*factor;
                m_textures[i].correction().rx() = l1/l2;
            }
            if (m_target == OGL::TargetRectangle)
                m_textures[i].correction() *= rect;
        }
    };

    auto addCustom = [this] (int plane, int width, int height,
                             const OpenGLTextureTransferInfo &fmt) {
        OpenGLTexture2D texture(m_target);
        texture.setAttributes(width, height, fmt);
        texture.plane() = plane;
        texture.create();
        if (m_params.imgfmt != IMGFMT_VDA) {
            if (m_dma) {
                glEnable(texture.target());
                texture.bind();
                glTexParameteri(texture.target(), GL_TEXTURE_STORAGE_HINT_APPLE,
                                GL_STORAGE_SHARED_APPLE);
//                glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
//                texture.initialize(frame.data(plane));
//                glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
            } else {
                texture.bind();
                texture.initialize();
            }
        }
        m_textures.append(texture);
    };

    auto add = [&] (int plane, OGL::TransferFormat fmt) {
        int components = 4;
        if (fmt == OGL::OneComponent)
            components = 1;
        else if (fmt == OGL::TwoComponents)
            components = 2;
        addCustom(plane, bytes[plane].width()/components, bytes[plane].height(),
                  OpenGLCompat::transferInfo(fmt));
    };

    auto getBits = [] (mp_imgfmt imgfmt) {
        switch (imgfmt) {
        case IMGFMT_420P16_LE: case IMGFMT_420P16_BE: return 16;
        case IMGFMT_420P14_LE: case IMGFMT_420P14_BE: return 14;
        case IMGFMT_420P12_LE: case IMGFMT_420P12_BE: return 12;
        case IMGFMT_420P10_LE: case IMGFMT_420P10_BE: return 10;
        case IMGFMT_420P9_LE:  case IMGFMT_420P9_BE:  return 9;
        default:                                      return 8;
        }
    };
    const int bits = getBits(m_imgfmtOut);
    const bool little = mpi->fmt.flags & MP_IMGFLAG_LE;
    m_texel = "vec3 texel(const in vec4 tex0) { return tex0.rgb; }";
    switch (m_imgfmtOut) {
    case IMGFMT_444P:
        add(0, OGL::OneComponent);
        add(1, OGL::OneComponent);
        add(2, OGL::OneComponent);
        m_texel = R"(
vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
    return vec3(tex0.r, tex1.r, tex2.r);
}
        )";
        break;
    case IMGFMT_420P:
        add(0, OGL::OneComponent);
        add(1, OGL::OneComponent);
        add(2, OGL::OneComponent);
        cc(2, 0.5);
        m_texel = R"(
vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
    return vec3(tex0.r, tex1.r, tex2.r);
}
        )";
        break;
    case IMGFMT_420P16_BE: case IMGFMT_420P16_LE:
    case IMGFMT_420P14_BE: case IMGFMT_420P14_LE:
    case IMGFMT_420P12_BE: case IMGFMT_420P12_LE:
    case IMGFMT_420P10_BE: case IMGFMT_420P10_LE:
    case IMGFMT_420P9_BE:  case IMGFMT_420P9_LE:
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
        add(0, OGL::TwoComponents);
        add(1, OGL::TwoComponents);
        add(2, OGL::TwoComponents);
        cc(2, 0.5);
        break;
    case IMGFMT_NV12: case IMGFMT_NV21: {
        m_texel = R"(
vec3 texel(const in vec4 tex0, const in vec4 tex1) {
    return vec3(tex0.r, tex1.!!);
}
        )";
        const auto rg = m_imgfmtOut == IMGFMT_NV12 ? "rg" : "gr";
        m_texel.replace("!!", OpenGLCompat::rg(rg));
        add(0, OGL::OneComponent); add(1, OGL::TwoComponents); cc(1, 0.5);
        break;
    } case IMGFMT_YUYV: case IMGFMT_UYVY:
        if (OpenGLCompat::hasExtension(OpenGLCompat::AppleYCbCr422)) {
            _Debug("Use GL_APPLE_ycbcr_422.");
            OpenGLTextureTransferInfo info;
            info.texture = OGL::RGB8_UNorm;
            info.transfer.format = OGL::YCbCr_422_Apple;
            info.transfer.type =  OGL::UInt16_8_8_Apple;
            if (m_imgfmtOut == IMGFMT_YUYV)
                info.transfer.type = OGL::UInt16_8_8_Rev_Apple;
            addCustom(0, m_params.w, m_params.h, info);
            m_cspOut = MP_CSP_RGB;
        } else if (OpenGLCompat::hasExtension(OpenGLCompat::MesaYCbCrTexture)) {
            _Debug("Use GL_MESA_ycbcr_texture.");
            m_texel = R"(
vec3 texel(const in int coord) { return texture0(coord).g!!; }
            )";
            m_texel.replace("!!", m_imgfmtOut == IMGFMT_YUYV ? "br" : "rb");
            OpenGLTextureTransferInfo info;
            info.texture = OGL::YCbCr_UNorm_Mesa;
            info.transfer.format = OGL::YCbCr_Mesa;
            info.transfer.type = OGL::UInt16_8_8_Mesa;
            if (m_imgfmtOut == IMGFMT_YUYV)
                info.transfer.type = OGL::UInt16_8_8_Rev_Mesa;
            addCustom(0, m_params.w, m_params.h, info);
        } else {
            m_texel = R"(
                vec3 texel(const in vec4 tex0, const in vec4 tex1) {
                    return vec3(tex0.?, tex1.!!);
                }
            )";
            m_texel.replace("?", OpenGLCompat::rg(IMGFMT_YUYV ? "r" : "g"));
            m_texel.replace("!!", m_imgfmtOut == IMGFMT_YUYV ? "ga" : "br");
            add(0, OGL::TwoComponents); add(0, OGL::BGRA);
            if (m_target == OGL::TargetRectangle)
                m_textures[1].correction().rx() *= 0.5;
        }
        break;
    case IMGFMT_BGRA: case IMGFMT_BGR0:
        add(0, OGL::BGRA);
        m_direct = true;
        break;
    case IMGFMT_RGBA: case IMGFMT_RGB0:
        add(0, OGL::RGBA);
        m_direct = true;
        break;
    case IMGFMT_ABGR: case IMGFMT_0BGR:
        add(0, OGL::BGRA);
        m_texel.replace(".rgb", ".arg");
        break;
    case IMGFMT_ARGB: case IMGFMT_0RGB:
        add(0, OGL::BGRA);
        m_texel.replace(".rgb", ".gra");
        break;
    default:
        break;
    }

    if (m_mixer) {
        m_mixer->create(m_textures);
        m_direct = m_mixer->directRendering();
        if (m_direct)
            m_cspOut = MP_CSP_RGB;
    }
}
