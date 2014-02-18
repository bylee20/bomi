#include "videoframeshader.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"
#include "log.hpp"
#ifdef Q_OS_MAC
#include <OpenGL/CGLIOSurface.h>
#include <OpenGL/OpenGL.h>
#include <CoreVideo/CVPixelBuffer.h>
#endif

DECLARE_LOG_CONTEXT(VideoFrameShader)

static const QByteArray shaderTemplate(
#include "videoframeshader.glsl.hpp"
);

VideoFrameShader::VideoFrameShader() {
	auto ctx = QOpenGLContext::currentContext();
	m_dma = ctx->hasExtension("GL_APPLE_client_storage") && ctx->hasExtension("GL_APPLE_texture_range");
	if (m_dma)
		qDebug() << "Use direct memory access.";
	m_vPositions = makeArray({-1.0, -1.0}, {1.0, 1.0});
	m_vMatrix.setToIdentity();
	m_lutInt[0].generate();
	m_lutInt[1].generate();
}

VideoFrameShader::~VideoFrameShader() {
	release();
}

void VideoFrameShader::release() {
	if (m_mixer) {
		m_textures[0].bind();
		_Delete(m_mixer);
		m_textures[0].unbind();
	}
	for (auto &texture : m_textures)
		texture.delete_();
	m_textures.clear();
}

void VideoFrameShader::updateTexCoords() {
	QPointF p1 = {0.0, 0.0}, p2 = {1.0, 1.0};
	if (m_target == OGL::Target2D)
		p2.rx() = _Ratio(m_frame.format().width(), m_frame.format().alignedWidth());
	else
		p2 = QPointF(m_frame.format().width(), m_frame.format().height());
	if (m_frame.isFlipped())
		qSwap(p1.ry(), p2.ry());
	m_vCoords = makeArray(p1, p2);
	m_coords = {p1, p2};
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
	color.matrix(m_mul_mat, m_add_vec, m_csp, range, m_bitScale);
	if (m_effects & VideoRendererItem::InvertColor) {
		m_mul_mat *= -1;
		m_add_vec = QVector3D(1, 1, 1) - m_add_vec;
	}
}

void VideoFrameShader::setEffects(int effects) {
	if (m_effects == effects)
		return;
	const int old = m_effects;
	m_effects = effects;
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
	if (m_frame.isInterlaced()) {
		switch (m_deint) {
		case DeintMethod::Bob:
			if (m_frame.format().imgfmt() != IMGFMT_VAAPI)
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
		auto declareVec2 = [] (const QString &name, const QPointF &p) {
			return _L("const vec2 ") + name + _L(" = vec2(") + _N(p.x(), 6) + _L(", ") + _N(p.y(), 6) + _L(");\n");
		};
		auto cc2string = [declareVec2, this] (int i) -> QString {
			QPointF cc = {1.0, 1.0};
			if (i < m_textures.size())
				cc = m_textures[i].cc;
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
#define sampler2D sampler2DRect
#define texture2D texture2DRect
#else
const vec4 dxdy = vec4(1.0/texWidth, 1.0/texHeight, -1.0/texWidth, 0.0);
const vec2 chroma_offset = chroma_location*dxdy.xy;
#endif
const vec2 dxy = dxdy.xy;
const vec2 tex_size = vec2(texWidth, texHeight);
)";

		m_lutCount = shader.interpolator->textures();
		Q_ASSERT(0 <= m_lutCount && m_lutCount < 3);

		shader.interpolator->allocate(m_lutInt[0], m_lutInt[1]);
		auto common = shader.interpolator->shader() + shaderTemplate;
		auto fragCode = header;
		fragCode += "#define FRAGMENT\n";
		fragCode += common;
		fragCode += m_texel;

		auto vertexCode = header;
		vertexCode += "#define VERTEX\n";
		vertexCode += common;

		prog.addShaderFromSourceCode(QOpenGLShader::Fragment, fragCode);
		prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexCode);

		prog.bindAttributeLocation("vPosition", vPosition);
		prog.bindAttributeLocation("vCoord", vCoord);

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
			loc_lut_int_mul[i] = m_prog->uniformLocation(name + "_mul");
		}
	}
}

bool VideoFrameShader::upload(VideoFrame &frame) {
	if (frame.isAdditional())
		return false;
	bool changed = m_frame.format() != frame.format();
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
#ifdef Q_OS_MAC
	if (m_frame.format().imgfmt() == IMGFMT_VDA) {
		for (const VideoTexture &texture : m_textures) {
			const auto cgl = CGLGetCurrentContext();
			const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)m_frame.data(3));
			texture.bind();
			const auto w = IOSurfaceGetWidthOfPlane(surface, texture.plane);
			const auto h = IOSurfaceGetHeightOfPlane(surface, texture.plane);
			CGLTexImageIOSurface2D(cgl, texture.target, texture.format.internal, w, h, texture.format.pixel, texture.format.type, surface, texture.plane);
			texture.unbind();
		}
		return changed;
	}
#endif
#ifdef Q_OS_LINUX
	if (m_mixer) {
		m_textures[0].bind();
		m_mixer->upload(m_frame, m_deint != DeintMethod::None);
		m_textures[0].unbind();
		return changed;
	}
#endif
	if (m_dma)
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
	for (int i=0; i<m_textures.size(); ++i)
		m_textures[i].upload2D(m_frame.data(m_textures[i].plane));
	if (m_dma)
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
	return changed;
}

void VideoFrameShader::render(const Kernel3x3 &k3x3) {
	if (!m_prog)
		return;
	glViewport(0, 0, m_frame.format().width(), m_frame.format().height());

	m_prog->bind();
	m_prog->setUniformValue(loc_top_field, (float)m_frame.isTopField());
	m_prog->setUniformValue(loc_add_vec, m_add_vec);
	m_prog->setUniformValue(loc_mul_mat, m_mul_mat);
	m_prog->setUniformValue(loc_vMatrix, m_vMatrix);
	if (hasKernelEffects()) {
		m_prog->setUniformValue(loc_kern_c, k3x3.center());
		m_prog->setUniformValue(loc_kern_n, k3x3.neighbor());
		m_prog->setUniformValue(loc_kern_d, k3x3.diagonal());
	}

	auto f = QOpenGLContext::currentContext()->functions();
	auto texPos = 0;
	for (int i=0; i<m_textures.size(); ++i, ++texPos) {
		m_prog->setUniformValue(loc_tex[i], texPos);
		m_prog->setUniformValue(loc_cc[i], m_textures[i].cc);
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		m_textures[i].bind();
	}
	for (int i=0; i<m_lutCount; ++i, ++texPos) {
		m_prog->setUniformValue(loc_lut_int[i], texPos);
		m_prog->setUniformValue(loc_lut_int_mul[i], m_lutInt[i].multiply);
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		m_lutInt[i].bind();
	}

	m_prog->enableAttributeArray(vCoord);
	m_prog->enableAttributeArray(vPosition);

	m_prog->setAttributeArray(vCoord, m_vCoords.data(), 2);
	m_prog->setAttributeArray(vPosition, m_vPositions.data(), 2);

	f->glActiveTexture(GL_TEXTURE0);

    glDisable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	m_prog->disableAttributeArray(0);
	m_prog->disableAttributeArray(1);
	m_prog->release();
}


void VideoFrameShader::fillInfo() {
	release();
	const auto &format = m_frame.format();
	m_shaders[0].rebuild = m_shaders[1].rebuild = true;

	m_bitScale = 1.0/255.0;
	m_csp = format.colorspace();
	m_target = m_dma ? OGL::TargetRectangle : OGL::Target2D;
	auto cc = [this, &format] (int factor, double rect) {
		for (int i=1; i<m_textures.size(); ++i) {
			if (format.imgfmt() != IMGFMT_VDA && i < format.planes())
				m_textures[i].cc.rx() = (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*factor);
			if (m_target == OGL::TargetRectangle)
				m_textures[i].cc *= rect;
		}
	};
	const int bits = format.encodedBits();
	const bool little = format.isLittleEndian();
	auto ctx = QOpenGLContext::currentContext();

	auto addCustom = [this, &format] (int plane, int width, int height, const OpenGLTextureFormat &fmt) {
		VideoTexture texture;
		texture.target = m_target;
		texture.plane = plane;
		texture.width = width;
		texture.height = height;
		texture.format = fmt;
		texture.generate();
		if (format.imgfmt() != IMGFMT_VDA) {
			if (m_dma) {
				glEnable(texture.target);
				texture.bind();
				glTexParameteri(texture.target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);
				glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
				texture.allocate(m_frame.data(plane));
				glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
			} else
				texture.allocate();
		}
		m_textures.append(texture);
	};

	auto add = [this, addCustom, &format] (int plane, GLenum fmt) {
		addCustom(plane, format.bytesPerLine(plane)/qMin<int>(4, fmt), format.lines(plane), OpenGLCompat::textureFormat(fmt));
	};

	m_texel = "vec3 texel(const in vec4 tex0) {return tex0.rgb;}"; // passthrough
	switch (format.type()) {
	case IMGFMT_444P:
		add(0, 1); add(1, 1); add(2, 1);
		m_texel = R"(
			vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
				return vec3(tex0.r, tex1.r, tex2.r);
			}
		)";
		break;
	case IMGFMT_420P:
		add(0, 1); add(1, 1); add(2, 1); cc(2, 0.5);
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
		add(0, 2); add(1, 2); add(2, 2); cc(2, 0.5);
		break;
	case IMGFMT_NV12:
	case IMGFMT_NV21:
		m_texel = R"(
			vec3 texel(const in vec4 tex0, const in vec4 tex1) {
				return vec3(tex0.r, tex1.!!);
			}
		)";
		m_texel.replace("!!", OpenGLCompat::rg(format.type() == IMGFMT_NV12 ? "rg" : "gr"));
		add(0, 1); add(1, 2); cc(1, 0.5);
		break;
	case IMGFMT_YUYV:
	case IMGFMT_UYVY:
		if (ctx->hasExtension("GL_APPLE_ycbcr_422")) {
			_Debug("Good! We have GL_APPLE_ycbcr_422.");
			OpenGLTextureFormat fmt;
			fmt.internal = OGL::RGB8_UNorm;
			fmt.pixel = OGL_YCbCr_422_Apple;
			fmt.type = format.type() == IMGFMT_YUYV ? OGL_UInt16_Rev_Apple : OGL_UInt16_Apple;
			addCustom(0, format.width(), format.height(), fmt);
			m_csp = MP_CSP_RGB;
		} else if (ctx->hasExtension("GL_MESA_ycbcr_texture")) {
			_Debug("Good! We have GL_MESA_ycbcr_texture.");
			m_texel = R"(vec3 texel(const in int coord) { return texture0(coord).g!!; })";
			m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "br" : "rb");
			OpenGLTextureFormat fmt;
			fmt.internal = OGL_YCbCr_UNorm_Mesa;
			fmt.pixel = OGL_YCbCr_Mesa;
			fmt.type = format.type() == IMGFMT_YUYV ? OGL_UInt16_Rev_Mesa : OGL_UInt16_Mesa;
			addCustom(0, format.width(), format.height(), fmt);
		} else {
			m_texel = R"(
				vec3 texel(const in vec4 tex0, const in vec4 tex1) {
					return vec3(tex0.?, tex1.!!);
				}
			)";
			m_texel.replace("?", format.type() == IMGFMT_YUYV ? "r" : OpenGLCompat::rg("g"));
			m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "ga" : "br");
			add(0, 2); add(0, 4);
			if (m_target == OGL::TargetRectangle)
				m_textures[1].cc.rx() *= 0.5;
		}
		break;
	case IMGFMT_BGRA:
		add(0, GL_BGRA);
		break;
	case IMGFMT_RGBA:
		add(0, GL_RGBA);
		break;
	case IMGFMT_ABGR:
		add(0, GL_BGRA);
		m_texel.replace(".rgb", ".arg");
		break;
	case IMGFMT_ARGB:
		add(0, GL_BGRA);
		m_texel.replace(".rgb", ".gra");
	default:
		break;
	}
#ifdef Q_OS_LINUX
	switch (format.imgfmt()) {
	case IMGFMT_VAAPI:
	case IMGFMT_VDPAU:
		Q_ASSERT(format.type() == IMGFMT_BGRA);
		Q_ASSERT(m_mixer == nullptr);
		m_csp = MP_CSP_RGB;
		m_textures[0].bind();
		m_mixer = HwAcc::createMixer(m_textures[0], format);
		m_textures[0].unbind();
		break;
	default:
		break;
	}
#endif
}
