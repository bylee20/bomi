#include "videoframeshader.hpp"
#include "hwacc.hpp"
#include "kernel3x3.hpp"
#include "misc/log.hpp"
#include "tmp/static_op.hpp"
#include "opengl/openglvertex.hpp"
#include "opengl/interpolator.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "enum/interpolatortype.hpp"

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
    m_target = m_dma ? OGL::TargetRectangle : OGL::Target2D;
    m_binding = OGL::bindingTarget(m_target);

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
    if (m_textures.isEmpty())
        return;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    for (auto &tex : m_textures)
        delete tex;
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
        p2.rx() = m_params.w/(double)m_textures.front()->width();
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

auto VideoFrameShader::setColor(const VideoColor &color,
                                ColorSpace space, ColorRange range) -> void
{
    m_color = color;
    m_space = space;
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
    if (m_effects.contains(VideoEffect::Gray))
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
    auto csp = m_cspOut;
    if (csp != MP_CSP_RGB && m_space != ColorSpace::Auto)
        csp = _EnumData(m_space);
    m_mul_mat = color.matrix(csp, range);
    if (m_effects.contains(VideoEffect::Invert))
        m_mul_mat = QMatrix4x4(-1,  0,  0, 1,
                                0, -1,  0, 1,
                                0,  0, -1, 1,
                                0,  0,  0, 1)*m_mul_mat;
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
                  + sizeNum(m_textures.front()->width()) + ";\n";
        header += "const float texHeight = "
                  + sizeNum(m_textures.front()->height()) + ";\n";
        auto declareVec2 = [] (const QString &name,
                               const QPointF &p) -> QString {
            return "const vec2 "_a % name % " = vec2("_a
                   % _N(p.x(), 6) % ", "_a % _N(p.y(), 6) % ");\n"_a;
        };
        auto cc2string = [declareVec2, this] (int i) -> QString {
            QPointF cc = {1.0, 1.0};
            if (i < m_textures.size())
                cc = m_textures[i]->correction();
            return declareVec2("cc"_a % _N(i), cc);
        };
        header += cc2string(1).toLatin1();
        header += cc2string(2).toLatin1();
        const auto chromaLeft = m_params.chroma_location == MP_CHROMA_LEFT;
        const qreal chroma_x = chromaLeft ? -0.5 : 0.0;
        header += declareVec2(u"chroma_location"_q, {chroma_x, 0.0});
        if (m_target != OGL::Target2D)
            header += "#define USE_RECTANGLE\n"_b;
        if (hasKernelEffects())
            header += "#define USE_KERNEL3x3\n"_b;

        header += "#define USE_DEINT "_b + QByteArray::number(deint) + '\n';
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
)"_b;
        auto interpolator = m_cspOut != MP_CSP_RGB ? shader.interpolator
            : Interpolator::get(InterpolatorType::Bilinear);
        m_lutCount = interpolator->textures();
        Q_ASSERT(0 <= m_lutCount && m_lutCount < 3);

        interpolator->allocate(&m_lutInt[0], &m_lutInt[1]);
        auto common = interpolator->shader() + shaderTemplate;
        auto fragCode = header;
        fragCode += "#define FRAGMENT\n"_b;
        fragCode += common;
        fragCode += m_texel;

        auto vertexCode = header;
        vertexCode += "#define VERTEX\n"_b;
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

auto VideoFrameShader::setFormat(const mp_image_params &params) -> void
{
    m_params = params;
    m_hwdec = IMGFMT_IS_HWACCEL(m_params.imgfmt);
    m_imgfmtOut = HwAcc::renderType(m_params.imgfmt);
    m_direct = false;
    if (m_imgfmtOut != IMGFMT_NONE) {
        m_direct = true;
        m_cspOut = MP_CSP_RGB;
    } else {
        m_imgfmtOut = m_params.imgfmt;
        m_cspOut = m_params.colorspace;
        switch (m_imgfmtOut) {
        case IMGFMT_BGRA: case IMGFMT_BGR0:
        case IMGFMT_RGBA: case IMGFMT_RGB0:
            m_direct = true; break;
        default: break;
        }
    }

    m_transferInfos.clear();
    auto add = [&] (OGL::TransferFormat fmt)
        { m_transferInfos.push_back(OpenGLTextureTransferInfo::get(fmt)); };

    const int bits = [&] () {
        switch (m_imgfmtOut) {
        case IMGFMT_420P16_LE: case IMGFMT_420P16_BE: return 16;
        case IMGFMT_420P14_LE: case IMGFMT_420P14_BE: return 14;
        case IMGFMT_420P12_LE: case IMGFMT_420P12_BE: return 12;
        case IMGFMT_420P10_LE: case IMGFMT_420P10_BE: return 10;
        case IMGFMT_420P9_LE:  case IMGFMT_420P9_BE:  return 9;
        default:                                      return 8;
        }
    }();
    const bool little = [&] () {
        switch (m_imgfmtOut) {
        case IMGFMT_420P16_LE: case IMGFMT_420P14_LE:
        case IMGFMT_420P12_LE: case IMGFMT_420P10_LE:
        case IMGFMT_420P9_LE:  return true;
        default:               return false;
        }
    }();
    m_texel = "vec3 texel(const in vec4 tex0) { return tex0.rgb; }";
    switch (m_imgfmtOut) {
    case IMGFMT_444P:
    case IMGFMT_420P:
        add(OGL::OneComponent);
        add(OGL::OneComponent);
        add(OGL::OneComponent);
        m_texel = R"(
            vec3 texel(const in vec4 tex0, const in vec4 tex1,
                       const in vec4 tex2)
            { return vec3(tex0.r, tex1.r, tex2.r); }
        )";
        break;
    case IMGFMT_420P16_BE: case IMGFMT_420P16_LE:
    case IMGFMT_420P14_BE: case IMGFMT_420P14_LE:
    case IMGFMT_420P12_BE: case IMGFMT_420P12_LE:
    case IMGFMT_420P10_BE: case IMGFMT_420P10_LE:
    case IMGFMT_420P9_BE:  case IMGFMT_420P9_LE:
        m_texel = R"(
            float convBits(const in vec4 tex)
            {
                const vec2 c = vec2(256.0, 1.0)/(256.0*??.0/255.0 + 1.0);
                return dot(tex.!!, c);
            }
            vec3 texel(const in vec4 tex0, const in vec4 tex1,
                       const in vec4 tex2)
            { return vec3(convBits(tex0), convBits(tex1), convBits(tex2)); }
        )";
        m_texel.replace("??", QByteArray::number((1 << (bits-8))-1));
        m_texel.replace("!!", OGL::rg(little ? "gr" : "rg"));
        add(OGL::TwoComponents);
        add(OGL::TwoComponents);
        add(OGL::TwoComponents);
        break;
    case IMGFMT_NV12: case IMGFMT_NV21: {
        m_texel = R"(
vec3 texel(const in vec4 tex0, const in vec4 tex1) {
    return vec3(tex0.r, tex1.!!);
}
        )";
        const auto rg = m_imgfmtOut == IMGFMT_NV12 ? "rg" : "gr";
        m_texel.replace("!!", OGL::rg(rg));
        add(OGL::OneComponent); add(OGL::TwoComponents);
        break;
    } case IMGFMT_YUYV: case IMGFMT_UYVY:
        if (OGL::hasExtension(OGL::AppleYCbCr422)) {
            _Debug("Use GL_APPLE_ycbcr_422.");
            OpenGLTextureTransferInfo info;
            info.texture = OGL::RGB8_UNorm;
            info.transfer.format = OGL::YCbCr_422_Apple;
            info.transfer.type =  OGL::UInt16_8_8_Apple;
            if (m_imgfmtOut == IMGFMT_YUYV)
                info.transfer.type = OGL::UInt16_8_8_Rev_Apple;
            m_transferInfos.push_back(info);
            m_cspOut = MP_CSP_RGB;
            m_direct = true;
        } else if (OGL::hasExtension(OGL::MesaYCbCrTexture)) {
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
            m_transferInfos.push_back(info);
        } else {
            m_texel = R"(
                vec3 texel(const in vec4 tex0, const in vec4 tex1) {
                    return vec3(tex0.?, tex1.!!);
                }
            )";
            m_texel.replace("?", OGL::rg(IMGFMT_YUYV ? "r" : "g"));
            m_texel.replace("!!", m_imgfmtOut == IMGFMT_YUYV ? "ga" : "br");
            add(OGL::TwoComponents); add(OGL::BGRA);
        }
        break;
    case IMGFMT_BGRA: case IMGFMT_BGR0:
        add(OGL::BGRA);
        break;
    case IMGFMT_RGBA: case IMGFMT_RGB0:
        add(OGL::RGBA);
        break;
    case IMGFMT_ABGR: case IMGFMT_0BGR:
        add(OGL::BGRA);
        m_texel.replace(".rgb", ".arg");
        break;
    case IMGFMT_ARGB: case IMGFMT_0RGB:
        add(OGL::BGRA);
        m_texel.replace(".rgb", ".gra");
        break;
    default:
        break;
    }

    m_refill = true;
}

auto VideoFrameShader::prepare(const mp_image *mpi) -> void
{
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
    static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;
    m_additional = mpi->fields & MP_IMGFIELD_ADDITIONAL;
    if (!m_additional) {
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
    m_top = mpi->fields & MP_IMGFIELD_TOP;
}

auto VideoFrameShader::upload(const mp_image *mpi, VideoTexture &texture) -> void
{
    if (!mpi || texture.isEmpty())
        return;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    if (m_hwdec || !m_additional) {
        texture.bind();
        texture.upload(mpi, m_deint != DeintMethod::None);
    }
}

auto VideoFrameShader::upload(const mp_image *mpi) -> void
{
    if (!mpi || m_textures.isEmpty())
        return;
    OpenGLTextureBaseBinder binder(m_target, m_binding);
    if (m_hwdec || !m_additional) {
        for (int i=0; i<m_textures.size(); ++i) {
            m_textures[i]->bind();
            m_textures[i]->upload(mpi, m_deint != DeintMethod::None);
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
    m_prog->setUniformValue(loc_mul_mat, m_mul_mat);
    if (hasKernelEffects()) {
        m_prog->setUniformValue(loc_kern_c, k3x3.center());
        m_prog->setUniformValue(loc_kern_n, k3x3.neighbor());
        m_prog->setUniformValue(loc_kern_d, k3x3.diagonal());
    }

    auto texPos = 0;
    for (int i=0; i<m_textures.size(); ++i, ++texPos) {
        m_textures[i]->bind(m_prog, loc_tex[i], texPos);
        m_prog->setUniformValue(loc_cc[i], m_textures[i]->correction());
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
    m_shaders[0].rebuild = m_shaders[1].rebuild = true;

    if (m_hwdec) {
        bytes.resize(1);
        bytes.front().rwidth() = tmp::aligned<4>(m_params.w)*4;
        bytes.front().rheight() = tmp::aligned<2>(m_params.h);
    }

    OpenGLTextureBaseBinder binder(m_target, m_binding);
    auto cc = [&] (int factor, double rect) {
        for (int i=1; i<m_textures.size(); ++i) {
            if (m_params.imgfmt != IMGFMT_VDA && i < bytes.size()) {
                const double l1 = bytes[0].width();
                const double l2 = bytes[i].width()*factor;
                m_textures[i]->correction().rx() = l1/l2;
            }
            if (m_target == OGL::TargetRectangle)
                m_textures[i]->correction() *= rect;
        }
    };

    int idx = 0;
    auto addCustom = [&] (int plane, int w, int h) {
        auto tex = new VideoTexture(m_target);
        tex->bind();
        tex->initialize(m_params.imgfmt, {w, h}, m_transferInfos[idx++],
                        m_params.imgfmt != IMGFMT_VDA && m_dma, plane);
        m_textures.push_back(tex);
    };

    auto add = [&] (int p, int comps)
        { addCustom(p, bytes[p].width()/comps, bytes[p].height()); };

    switch (m_imgfmtOut) {
    case IMGFMT_444P:       case IMGFMT_420P:
        add(0, 1); add(1, 1); add(2, 1);
        if (m_imgfmtOut == IMGFMT_420P)
            cc(2, 0.5);
        break;
    case IMGFMT_420P16_BE:  case IMGFMT_420P16_LE:
    case IMGFMT_420P14_BE:  case IMGFMT_420P14_LE:
    case IMGFMT_420P12_BE:  case IMGFMT_420P12_LE:
    case IMGFMT_420P10_BE:  case IMGFMT_420P10_LE:
    case IMGFMT_420P9_BE:   case IMGFMT_420P9_LE:
        add(0, 2); add(1, 2); add(2, 2); cc(2, 0.5);
        break;
    case IMGFMT_NV12:       case IMGFMT_NV21:
        add(0, 1); add(1, 2); cc(1, 0.5);
        break;
    case IMGFMT_YUYV:     case IMGFMT_UYVY:
        if (OGL::hasExtension(OGL::AppleYCbCr422)
                || OGL::hasExtension(OGL::MesaYCbCrTexture))
            addCustom(0, m_params.w, m_params.h);
        else {
            add(0, 2); add(0, 4);
            if (m_target == OGL::TargetRectangle)
                m_textures[1]->correction().rx() *= 0.5;
        }
        break;
    case IMGFMT_BGRA:       case IMGFMT_BGR0:
    case IMGFMT_RGBA:       case IMGFMT_RGB0:
    case IMGFMT_ABGR:       case IMGFMT_0BGR:
    case IMGFMT_ARGB:       case IMGFMT_0RGB:
        add(0, 4);
        break;
    default:
        break;
    }
}
