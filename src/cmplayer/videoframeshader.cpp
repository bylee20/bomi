#include "videoframeshader.hpp"
#include "videoframe.hpp"
#include "videoframeshader.glsl.hpp"
#ifdef Q_OS_LINUX
#include "hwacc_vaapi.hpp"
#include <va/va_glx.h>
#include <va/va_x11.h>
#endif

static const QByteArray shaderTemplate((const char*)videoframeshader_glsl, videoframeshader_glsl_len);

VideoFrameShader::VideoFrameShader(const VideoFormat &format, const ColorProperty &color, int effects, const DeintInfo &deint)
: m_format(format), m_color(color), m_effects(effects), m_deint(deint) {
	m_vMatrix.setToIdentity();
	m_vMatrix.ortho(0, format.width(), 0, format.height(), -1, 1);
	fillInfo();
	updateTexCoords();
	updateVertexPositions();
	updateColorMatrix();
}

VideoFrameShader::~VideoFrameShader() {
	if (m_vaSurfaceGLX) {
		m_textures[0].bind();
		vaDestroySurfaceGLX(VaApi::glx(), m_vaSurfaceGLX);
	}
	for (auto &texture : m_textures)
		texture.delete_();
}

void VideoFrameShader::updateTexCoords() {
	QPointF p1 = {0.0, 0.0}, p2 = {1.0, 1.0};
	if (m_target == GL_TEXTURE_2D)
		p2.rx() = _Ratio(m_format.width(), m_format.alignedWidth());
	else
		p2 = QPointF(m_format.width(), m_format.height());
	if (m_flipped)
		qSwap(p1.ry(), p2.ry());
	m_vCoords = makeArray(p1, p2);
}

void VideoFrameShader::setColor(const ColorProperty &color) {
	if (_Change(m_color, color))
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
	if (m_effects != effects) {
		const int old = m_effects;
		m_effects = effects;
		if ((old & VideoRendererItem::ColorEffects) != (m_effects & VideoRendererItem::ColorEffects))
			updateColorMatrix();
		m_rebuild = hasKernelEffects() != isKernelEffect(old);
	}
}

void VideoFrameShader::setDeintInfo(const DeintInfo &deint) {
	if (_Change(m_deint, deint))
		m_rebuild = true;
}

void VideoFrameShader::build() {
	m_rebuild = false;
	m_shader.removeAllShaders();

	QByteArray header;
	header += "#define TEX_COUNT " + QByteArray::number(m_textures.size()) + "\n";
	header += "const float texWidth = " + _N((double)m_format.alignedWidth(), 1).toLatin1() + ";\n";
	header += "const float texHeight = " + _N((double)m_format.alignedHeight(), 1).toLatin1() + ";\n";
	auto cc2string = [this] (int i) -> QString {
		QPointF cc = {1.0, 1.0};
		if (i < m_textures.size())
			cc = m_textures[i].cc;
		return _L("const vec2 cc") + _N(i) + _L(" = vec2(") + _N(cc.x(), 6) + _L(", ") + _N(cc.y(), 6) + _L(");\n");
	};
	header += cc2string(1).toLatin1();
	header += cc2string(2).toLatin1();
	if (m_target != GL_TEXTURE_2D || m_format.isEmpty())
		header += "#define USE_RECTANGLE\n";
	qDebug() << "has kernel?" << hasKernelEffects();
	if (hasKernelEffects())
		header += "#define USE_KERNEL3x3\n";
	if (m_deint.hardware() == DeintInfo::OpenGL) {
		if (m_deint.method() == DeintInfo::Bob)
			header += "#define USE_DEINT 1\n";
		else if (m_deint.method() == DeintInfo::LinearBob)
			header += "#define USE_DEINT 2\n";
	}

	QByteArray frag = header;
	frag += "#define FRAGMENT\n";
	frag += shaderTemplate;
	frag += m_texel;

	QByteArray vertex = header;
	vertex += "#define VERTEX\n";
	vertex += shaderTemplate;

	m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
	m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex);

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

void VideoFrameShader::upload(const VideoFrame &frame) {
	m_field = frame.field();
	if (_Change(m_flipped, frame.isFlipped()))
		updateTexCoords();
#ifdef Q_OS_LINUX
	if (m_format.imgfmt() == IMGFMT_VAAPI) {
		static const int specs[MP_CSP_COUNT] = {
			0,					//MP_CSP_AUTO,
			VA_SRC_BT601,		//MP_CSP_BT_601,
			VA_SRC_BT709,		//MP_CSP_BT_709,
			VA_SRC_SMPTE_240,	//MP_CSP_SMPTE_240M,
			0,					//MP_CSP_RGB,
			0,					//MP_CSP_XYZ,
			0,					//MP_CSP_YCGCO,
		};
		static const int field[] = { // Picture = 0, Top = 1, Bottom = 2
			VA_FRAME_PICTURE, VA_TOP_FIELD, VA_BOTTOM_FIELD, VA_FRAME_PICTURE
		};
		auto id = (VASurfaceID)(uintptr_t)frame.data(3);
		int flags = specs[m_format.colorspace()];
		if (m_deint.hardware() & DeintInfo::VaApi)
			flags |= field[m_field & VideoFrame::Interlaced];
		m_textures[0].bind();
		vaSyncSurface(VaApi::glx(), id);
		vaCopySurfaceGLX(VaApi::glx(), m_vaSurfaceGLX, id,  flags);
	} else
#endif
	for (int i=0; i<m_textures.size(); ++i)
		m_textures[i].upload2D(frame.data(i));
}

void VideoFrameShader::render(const Kernel3x3 &k3x3) {
	glViewport(0, 0, m_format.width(), m_format.height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	m_shader.bind();
	m_shader.setUniformValue(loc_top_field, float(!!(m_field & VideoFrame::Top)));
	m_shader.setUniformValue(loc_deint, float(!!(m_field & VideoFrame::Interlaced)));
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
	m_csp = m_format.colorspace();
	m_range = m_format.range();
	m_target = m_format.imgfmt() != IMGFMT_VDA ? GL_TEXTURE_2D : GL_TEXTURE_RECTANGLE;
	auto cc = [this] (int factor, double rect) {
		for (int i=1; i<m_textures.size(); ++i) {
			if (m_format.imgfmt() != IMGFMT_VDA && i < m_format.planes())
				m_textures[i].cc.rx() = (double)m_format.bytesPerLine(0)/(double)(m_format.bytesPerLine(i)*factor);
			if (m_target == GL_TEXTURE_RECTANGLE)
				m_textures[i].cc *= rect;
		}
	};
	const int bits = m_format.encodedBits();
	const bool little = m_format.isLittleEndian();
	auto ctx = QOpenGLContext::currentContext();

	auto addCustom = [this] (int plane, int width, int height, const OpenGLTextureFormat &format) {
		VideoTexture2 texture;
		texture.target = m_target;
		texture.plane = plane;
		texture.width = width;
		texture.height = height;
		texture.format = format;
		texture.generate();
		texture.allocate();
		m_textures.append(texture);
	};

	auto add = [this, addCustom] (int plane, GLenum format) {
		addCustom(plane, m_format.bytesPerLine(plane)/qMin<int>(4, format), m_format.lines(plane), OpenGLCompat::textureFormat(format));
	};

	m_texel = "vec3 texel(const in int coord) {return texture0(coord).rgb;}"; // passthrough
	switch (m_format.type()) {
	case IMGFMT_420P:
		add(0, 1); add(1, 1); add(2, 1); cc(2, 0.5);
		m_texel = R"(
			vec3 texel(const in int coord) {
				vec3 yuv;
				yuv.x = texture0(coord).r;
				yuv.y = texture1(coord).r;
				yuv.z = texture2(coord).r;
				return yuv;
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
			vec3 texel(const in vec2 coord) {
				vec3 yuv;
				yuv.x = convBits(texture0(coord));
				yuv.y = convBits(texture1(coord));
				yuv.z = convBits(texture2(coord));
				return yuv;
			}
		)";
		m_texel.replace("??", QByteArray::number((1 << (bits-8))-1));
		m_texel.replace("!!", OpenGLCompat::rg(little ? "gr" : "rg"));
		add(0, 2); add(1 ,2); add(2, 2); cc(2, 0.5);
		break;
	case IMGFMT_NV12:
	case IMGFMT_NV21:
		m_texel = R"(
			vec3 texel(const in int coord) {
				vec3 yuv;
				yuv.x = texture0(coord).r;
				yuv.yz = texture1(coord).!!;
				return yuv;
			}
		)";
		m_texel.replace("!!", OpenGLCompat::rg(m_format.type() == IMGFMT_NV12 ? "rg" : "gr"));
		add(0, 1); add(1, 2); cc(1, 0.5);
		break;
	case IMGFMT_YUYV:
	case IMGFMT_UYVY:
#ifdef Q_OS_MAC
		MAKE(AppleY422Shader) // "GL_APPLE_ycbcr_422"
#else
		if (ctx->hasExtension("GL_MESA_ycbcr_texture")) {
			qDebug() << "Good! We have GL_MESA_ycbcr_texture.";
			m_texel = R"(vec3 texel(const in int coord) { return texture0(coord).g!!; })";
			m_texel.replace("!!", m_format.type() == IMGFMT_YUYV ? "br" : "rb");
			OpenGLTextureFormat format;
			format.internal = format.pixel = GL_YCBCR_MESA;
			format.type = m_format.type() == IMGFMT_YUYV ? GL_UNSIGNED_SHORT_8_8_REV_MESA : GL_UNSIGNED_SHORT_8_8_MESA;
			addCustom(0, m_format.width(), m_format.height(), format);
			break;
		}
#endif
		m_texel = R"(
			vec3 texel(const in int coord) {
				vec3 yuv;
				yuv.x = texture0(coord).?;
				yuv.yz = texture1(coord).!!;
				return yuv;
			}
		)";
		m_texel.replace("?", m_format.type() == IMGFMT_YUYV ? "r" : OpenGLCompat::rg("g"));
		m_texel.replace("!!", m_format.type() == IMGFMT_YUYV ? "ga" : "br");
		add(0, 2); add(0, 4); cc(1, 0.5);
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
	if (m_format.imgfmt() == IMGFMT_VAAPI) {
		Q_ASSERT(m_format.type() == IMGFMT_BGRA);
		m_csp = MP_CSP_RGB;
		m_textures[0].bind();
		vaCreateSurfaceGLX(VaApi::glx(), m_textures[0].target, m_textures[0].id, &m_vaSurfaceGLX);
	}
#endif
}

//bool VideoFrameShader::tryPixmap() {
//#define GET_PROC(a) a = (decltype(a))QOpenGLContext::currentContext()->getProcAddress(#a)
//	GET_PROC(glXBindTexImageEXT);
//	GET_PROC(glXReleaseTexImageEXT);
//#undef GET_PROC
//	if (!glXBindTexImageEXT || !glXReleaseTexImageEXT)
//		return false;
//	auto dpy = QX11Info::display();
//	auto root = QX11Info::appRootWindow(QX11Info::appScreen());
//	XWindowAttributes xattrs;
//	XGetWindowAttributes(dpy, root, &xattrs);
//	if (xattrs.depth != 24 && xattrs.depth != 32)
//		return false;
//	m_pixmap = XCreatePixmap(dpy, root, m_format.width(), m_format.height(), xattrs.depth);
//	if (!m_pixmap)
//		return false;

//	QVector<int> fbattrs(30, GL_NONE);
//	auto pattrs = fbattrs.data();
//	auto push = [&pattrs] (int key, int value) { *pattrs++ = key; *pattrs++ = value; };
//	push(GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT);
//	push(GLX_DOUBLEBUFFER, GL_TRUE);
//	push(GLX_RENDER_TYPE, GLX_RGBA_BIT);
//	push(GLX_X_RENDERABLE, GL_TRUE);
//	push(GLX_Y_INVERTED_EXT, GL_TRUE);
//	push(GLX_RED_SIZE,8);
//	push(GLX_GREEN_SIZE, 8);
//	push(GLX_BLUE_SIZE, 8);
//	push(GLX_DEPTH_SIZE, xattrs.depth);
//	if (xattrs.depth == 32) {
//		push(GLX_ALPHA_SIZE, 8);
//		push(GLX_BIND_TO_TEXTURE_RGBA_EXT, GL_TRUE);
//	} else
//		push(GLX_BIND_TO_TEXTURE_RGB_EXT, GL_TRUE);
//	int num = fbattrs.size();
//	auto config = glXChooseFBConfig(dpy, QX11Info::appScreen(), fbattrs.data(), &num);
//	if (!config)
//		return false;

//	QVector<int> pxattrs(10, GL_NONE);
//	pattrs = pxattrs.data();
//	push(GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT);
//	push(GLX_MIPMAP_TEXTURE_EXT, GL_FALSE);
//	push(GLX_TEXTURE_FORMAT_EXT, xattrs.depth == 32 ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT);
//	m_glxPixmap = glXCreatePixmap(dpy, *config, m_pixmap, pxattrs.data());
//	free(config);

//	qDebug() <<m_format.width() << _Aligned<16>(m_format.width());
//	return m_glxPixmap;
//}
