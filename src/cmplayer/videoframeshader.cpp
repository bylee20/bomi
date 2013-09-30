#include "videoframeshader.hpp"
#include "videoframe.hpp"
#include "videoframeshader.glsl.hpp"
#ifdef Q_OS_LINUX
#include "hwacc_vaapi.hpp"
#include <va/va_glx.h>
#include <va/va_x11.h>
#endif
#ifdef Q_OS_MAC
#include <OpenGL/CGLIOSurface.h>
#include <OpenGL/OpenGL.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <qpa/qplatformnativeinterface.h>
#endif

static const QByteArray shaderTemplate((const char*)videoframeshader_glsl, videoframeshader_glsl_len);

VideoFrameShader::VideoFrameShader() {
	auto ctx = QOpenGLContext::currentContext();
	m_dma = ctx->hasExtension("GL_APPLE_client_storage") && ctx->hasExtension("GL_APPLE_texture_range");
	if (m_dma)
		qDebug() << "Use direct memory access.";
	m_vPositions = makeArray({-1.0, -1.0}, {1.0, 1.0});
	m_vMatrix.setToIdentity();
}

VideoFrameShader::~VideoFrameShader() {
	release();
}

void VideoFrameShader::release() {
#ifdef Q_OS_LINUX
	if (m_vaSurfaceGLX) {
		m_textures[0].bind();
		vaDestroySurfaceGLX(VaApi::glx(), m_vaSurfaceGLX);
		m_vaSurfaceGLX = nullptr;
	}
#endif
	for (auto &texture : m_textures)
		texture.delete_();
	m_textures.clear();
}

void VideoFrameShader::updateTexCoords() {
	QPointF p1 = {0.0, 0.0}, p2 = {1.0, 1.0};
	if (m_target == GL_TEXTURE_2D)
		p2.rx() = _Ratio(m_frame.format().width(), m_frame.format().alignedWidth());
	else
		p2 = QPointF(m_frame.format().width(), m_frame.format().height());
	if (m_frame.isFlipped())
		qSwap(p1.ry(), p2.ry());
	m_vCoords = makeArray(p1, p2);
	m_coords = {p1, p2};
}

void VideoFrameShader::setColor(const ColorProperty &color) {
	m_color = color;
	updateColorMatrix();
}

void VideoFrameShader::updateColorMatrix() {
	auto color = m_color;
	if (m_effects & VideoRendererItem::Grayscale)
		color.setSaturation(-1.0);
	auto mat = color.matrix(m_csp, m_range);
	m_mul_mat = mat.toGenericMatrix<3, 3>().transposed();
	m_add_vec = mat.column(3).toVector3D();
	m_sub_vec = mat.row(3).toVector3D();
	if (m_effects & VideoRendererItem::InvertColor) {
		m_mul_mat *= -1;
		m_add_vec += QVector3D(1, 1, 1);
	}
}

void VideoFrameShader::setEffects(int effects) {
	if (m_effects == effects)
		return;
	const int old = m_effects;
	m_effects = effects;
	if (isColorEffect(old) != isColorEffect(m_effects))
		updateColorMatrix();
	m_rebuild = hasKernelEffects() != isKernelEffect(old);
}

void VideoFrameShader::setDeintMethod(DeintMethod method) {
	if (_Change(m_deint, method))
		m_rebuild = true;
}

void VideoFrameShader::build() {
	const VideoFormat &format = m_frame.format();
	if (format.isEmpty())
		return;
	m_shader.removeAllShaders();
	m_rebuild = false;

	QByteArray header;
	header += "#define TEX_COUNT " + QByteArray::number(m_textures.size()) + "\n";
	header += "const float texWidth = " + _N((double)format.alignedWidth(), 1).toLatin1() + ";\n";
	header += "const float texHeight = " + _N((double)format.alignedHeight(), 1).toLatin1() + ";\n";
	auto cc2string = [this] (int i) -> QString {
		QPointF cc = {1.0, 1.0};
		if (i < m_textures.size())
			cc = m_textures[i].cc;
		return _L("const vec2 cc") + _N(i) + _L(" = vec2(") + _N(cc.x(), 6) + _L(", ") + _N(cc.y(), 6) + _L(");\n");
	};
	header += cc2string(1).toLatin1();
	header += cc2string(2).toLatin1();
	if (m_target != GL_TEXTURE_2D || format.isEmpty())
		header += "#define USE_RECTANGLE\n";
	if (hasKernelEffects())
		header += "#define USE_KERNEL3x3\n";
	int deint = 0;
	switch (m_deint) {
	case DeintMethod::Bob:
		if (format.imgfmt() != IMGFMT_VAAPI)
			deint = 1;
		break;
	case DeintMethod::LinearBob:
		deint = 2;
		break;
	default:
		break;
	}
	header += "#define USE_DEINT " + QByteArray::number(deint) + "\n";
	qDebug() << "deint" << deint;

	m_fragCode = header;
	m_fragCode += "#define FRAGMENT\n";
	m_fragCode += shaderTemplate;
	m_fragCode += m_texel;

	m_vertexCode = header;
	m_vertexCode += "#define VERTEX\n";
	m_vertexCode += shaderTemplate;

	m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, m_fragCode);
	m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, m_vertexCode);

	m_shader.bindAttributeLocation("vPosition", vPosition);
	m_shader.bindAttributeLocation("vCoord", vCoord);

	m_shader.link();
	loc_tex[0] = m_shader.uniformLocation("tex0");
	loc_tex[1] = m_shader.uniformLocation("tex1");
	loc_tex[2] = m_shader.uniformLocation("tex2");
	loc_top_field = m_shader.uniformLocation("top_field");
	loc_deint = m_shader.uniformLocation("deint");
	loc_mul_mat = m_shader.uniformLocation("mul_mat");
	loc_sub_vec = m_shader.uniformLocation("sub_vec");
	loc_add_vec = m_shader.uniformLocation("add_vec");
	loc_cc[0] = m_shader.uniformLocation("cc0");
	loc_cc[1] = m_shader.uniformLocation("cc1");
	loc_cc[2] = m_shader.uniformLocation("cc2");
	loc_vMatrix = m_shader.uniformLocation("vMatrix");
	loc_kern_c = m_shader.uniformLocation("kern_c");
	loc_kern_d = m_shader.uniformLocation("kern_d");
	loc_kern_n = m_shader.uniformLocation("kern_n");
}

bool VideoFrameShader::upload(VideoFrame &frame) {
	if (frame.isAdditional())
		return false;
	bool changed = m_frame.format() != frame.format();
	if (m_dma) {
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
	if (m_rebuild)
		build();
	if (m_rebuild)
		return false;
#ifdef Q_OS_MAC
	if (m_format.imgfmt() == IMGFMT_VDA) {
		for (const VideoTexture2 &texture : m_textures) {
			const auto cgl = static_cast<CGLContextObj>(qApp->platformNativeInterface()->nativeResourceForContext("cglcontextobj", QOpenGLContext::currentContext()));
			const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)m_frame.data(3));
			texture.bind();
			const auto w = IOSurfaceGetWidthOfPlane(surface, texture.plane);
			const auto h = IOSurfaceGetHeightOfPlane(surface, texture.plane);
			CGLTexImageIOSurface2D(cgl, texture.target, texture.format.internal, w, h, texture.format.pixel, texture.format.type, surface, texture.plane);
			texture.unbind();
		}
		return ret;
	}
#endif
#ifdef Q_OS_LINUX
	if (m_frame.format().imgfmt() == IMGFMT_VAAPI) {
		static const int specs[MP_CSP_COUNT] = {
			0,					//MP_CSP_AUTO,
			VA_SRC_BT601,		//MP_CSP_BT_601,
			VA_SRC_BT709,		//MP_CSP_BT_709,
			VA_SRC_SMPTE_240,	//MP_CSP_SMPTE_240M,
			0,					//MP_CSP_RGB,
			0,					//MP_CSP_XYZ,
			0,					//MP_CSP_YCGCO,
		};
		static const int field[] = {
			// Picture = 0,   Top = 1,      Bottom = 2
			VA_FRAME_PICTURE, VA_TOP_FIELD, VA_BOTTOM_FIELD, VA_FRAME_PICTURE
		};
		auto id = (VASurfaceID)(uintptr_t)m_frame.data(3);
		int flags = specs[m_frame.format().colorspace()];
		if (m_deint == DeintMethod::Bob)
			flags |= field[m_frame.field() & VideoFrame::Interlaced];
		m_textures[0].bind();
		vaCopySurfaceGLX(VaApi::glx(), m_vaSurfaceGLX, id,  flags);
		vaSyncSurface(VaApi::glx(), id);
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
	if (m_rebuild)
		return;
	glViewport(0, 0, m_frame.format().width(), m_frame.format().height());
//  VideoFrame is always opaque. No need to clear background.
//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//	glClear(GL_COLOR_BUFFER_BIT);

	m_shader.bind();
	m_shader.setUniformValue(loc_top_field, (float)m_frame.isTopField());
	m_shader.setUniformValue(loc_deint, (float)m_frame.isInterlaced());
	m_shader.setUniformValue(loc_sub_vec, m_sub_vec);
	m_shader.setUniformValue(loc_add_vec, m_add_vec);
	m_shader.setUniformValue(loc_mul_mat, m_mul_mat);
	m_shader.setUniformValue(loc_vMatrix, m_vMatrix);
	if (hasKernelEffects()) {
		m_shader.setUniformValue(loc_kern_c, k3x3.center());
		m_shader.setUniformValue(loc_kern_n, k3x3.neighbor());
		m_shader.setUniformValue(loc_kern_d, k3x3.diagonal());
	}

	auto f = QOpenGLContext::currentContext()->functions();
	for (int i=0; i<m_textures.size(); ++i) {
		m_shader.setUniformValue(loc_tex[i], i);
		m_shader.setUniformValue(loc_cc[i], m_textures[i].cc);
		f->glActiveTexture(GL_TEXTURE0 + i);
		m_textures[i].bind();
	}

	m_shader.enableAttributeArray(vCoord);
	m_shader.enableAttributeArray(vPosition);

	m_shader.setAttributeArray(vCoord, m_vCoords.data(), 2);
	m_shader.setAttributeArray(vPosition, m_vPositions.data(), 2);

	f->glActiveTexture(GL_TEXTURE0);
	glDrawArrays(GL_QUADS, 0, 4);

	m_shader.disableAttributeArray(0);
	m_shader.disableAttributeArray(1);
	m_shader.release();
}


void VideoFrameShader::fillInfo() {
	release();
	const auto &format = m_frame.format();
	m_rebuild = true;
	m_csp = format.colorspace();
	m_range = format.range();
	m_target = m_dma ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D;
	auto cc = [this, &format] (int factor, double rect) {
		for (int i=1; i<m_textures.size(); ++i) {
			if (format.imgfmt() != IMGFMT_VDA && i < format.planes())
				m_textures[i].cc.rx() = (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*factor);
			if (m_target == GL_TEXTURE_RECTANGLE)
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
				texture.allocate(GL_LINEAR, m_frame.data(plane));
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
	case IMGFMT_420P:
		add(0, 1); add(1, 1); add(2, 1); cc(2, 0.5);
		m_texel = R"(
			vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
				return vec3(tex0.r, tex1.r, tex2.r);
			}
		)";
		break;
	case IMGFMT_420P16_LE:
	case IMGFMT_420P16_BE:
	case IMGFMT_420P14_LE:
	case IMGFMT_420P14_BE:
	case IMGFMT_420P12_LE:
	case IMGFMT_420P12_BE:
	case IMGFMT_420P10_LE:
	case IMGFMT_420P10_BE:
	case IMGFMT_420P9_LE:
	case IMGFMT_420P9_BE:
		m_texel = R"(
			float convBits(const in vec4 tex) {
				const vec2 c = vec2(265.0, 1.0)/(256.0*??.0/255.0 + 1.0);
				return dot(tex.!!, c);
			}
			vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2) {
				return vec3(convBits(tex0), convBits(tex1), convBits(tex2));
			}
		)";
		m_texel.replace("??", QByteArray::number((1 << (bits-8))-1));
		m_texel.replace("!!", OpenGLCompat::rg(little ? "gr" : "rg"));
		add(0, 2); add(1 ,2); add(2, 2); cc(2, 0.5);
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
		if (!ctx->hasExtension("GL_APPLE_ycbcr_422")) {
			qDebug() << "Good! We have GL_APPLE_ycbcr_422.";
			OpenGLTextureFormat fmt;
			fmt.internal = GL_RGB8;
			fmt.pixel = GL_YCBCR_422_APPLE;
			fmt.type = format.type() == IMGFMT_YUYV ? GL_UNSIGNED_SHORT_8_8_REV_APPLE : GL_UNSIGNED_SHORT_8_8_APPLE;
			addCustom(0, format.width(), format.height(), fmt);
			m_csp = MP_CSP_RGB;
		} else if (ctx->hasExtension("GL_MESA_ycbcr_texture")) {
			qDebug() << "Good! We have GL_MESA_ycbcr_texture.";
			m_texel = R"(vec3 texel(const in int coord) { return texture0(coord).g!!; })";
			m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "br" : "rb");
			OpenGLTextureFormat fmt;
			fmt.internal = fmt.pixel = GL_YCBCR_MESA;
			fmt.type = format.type() == IMGFMT_YUYV ? GL_UNSIGNED_SHORT_8_8_REV_MESA : GL_UNSIGNED_SHORT_8_8_MESA;
			addCustom(0, format.width(), format.height(), fmt);
		} else {
			m_texel = R"(
				vec3 texel(const in vec4 tex0, const in vec4 tex1) {
					return vec3(tex0.?, tex1.!!);
					return vec3(tex0.r, tex1.gg);
				}
			)";
			m_texel.replace("?", format.type() == IMGFMT_YUYV ? "r" : OpenGLCompat::rg("g"));
			m_texel.replace("!!", format.type() == IMGFMT_YUYV ? "ga" : "br");
			add(0, 2); add(0, 4);
			if (m_target == GL_TEXTURE_RECTANGLE)
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
		break;
	default:
		break;
	}
#ifdef Q_OS_LINUX
	if (format.imgfmt() == IMGFMT_VAAPI) {
		Q_ASSERT(format.type() == IMGFMT_BGRA);
		m_csp = MP_CSP_RGB;
		m_textures[0].bind();
		vaCreateSurfaceGLX(VaApi::glx(), m_textures[0].target, m_textures[0].id, &m_vaSurfaceGLX);
	}
#endif
}

const char *const *VideoFrameShader::attributes() const {
	static const char *const names[] = {"vPosition", "vCoord", nullptr};
	return names;
}
