#include "textureshader.hpp"
#include "videoframe.hpp"

TextureShader::TextureShader(const VideoFormat &format, GLenum target)
: m_format(format), m_target(target) {
	m_info.reserve(3);
	m_header = R"(
		uniform float kern_c, kern_n, kern_d;
		uniform vec2 sc1, sc2, sc3;
		varying highp vec2 qt_TexCoord;
		uniform mat3 conv_mat;
		uniform vec3 conv_vec, orig_vec;
		vec3 texel(const in vec2 coord);
		vec3 get_texel(const in vec2 coord);
		vec3 get_color(const in vec2 coord);
	)";
	if (target == GL_TEXTURE_RECTANGLE_ARB) {
		m_header += R"(
			uniform sampler2DRect p1, p2, p3;
			vec4 texture1(const in vec2 coord) { return texture2DRect(p1, coord); }
			vec4 texture2(const in vec2 coord) { return texture2DRect(p2, coord*sc2); }
			vec4 texture3(const in vec2 coord) { return texture2DRect(p3, coord*sc3); }
		)";
	} else {
		m_header += R"(
			uniform sampler2D p1, p2, p3;
			vec4 texture1(const in vec2 coord) { return texture2D(p1, coord); }
			vec4 texture2(const in vec2 coord) { return texture2D(p2, coord*sc2); }
			vec4 texture3(const in vec2 coord) { return texture2D(p3, coord*sc3); }
		)";
	}
	if (m_target == GL_TEXTURE_2D)
		m_rect.setRight(_Ratio(format.width(), format.alignedWidth()));
	else
		m_rect.setBottomRight(QPointF(format.width(), format.height()));
}

void TextureShader::initialize(GLuint *textures) {
	m_textures = textures;
	for (int i=0; i<m_info.size(); ++i) {
		m_info[i].id = textures[i];
		m_uploader->initialize(m_info[i]);
	}
}

QByteArray TextureShader::fragment() const {
	QByteArray codes = m_header;
	codes += m_texel;
	if (m_effects & VideoRendererItem::KernelEffects) {
		QString dxy;
		const char *fmt = "const vec4 dxy = vec4(%f, %f, %f, %f);\n";
		if (m_target == GL_TEXTURE_2D && !format().isEmpty()) {
			const float dx = 1.0/(double)format().alignedWidth();
			const float dy = 1.0/(double)format().alignedHeight();
			dxy.sprintf(fmt, dx, dy, -dx, 0.f);
		} else
			dxy.sprintf(fmt, 1.f, 1.f, -1.f, 0.f);
		codes += dxy.toLatin1();
		codes += R"(
			vec3 get_texel(const in vec2 coord) {
				// dxy.zy   dxy.wy   dxy.xy
				// dxy.zw     0      dxy.xw
				//-dxy.xy  -dxy.wy  -dxy.zy
				vec3 c = texel(coord)*kern_c;
				c += (texel(coord + dxy.wy)+texel(coord + dxy.zw)+texel(coord + dxy.xw)+texel(coord - dxy.wy))*kern_n;
				c += (texel(coord + dxy.zy)+texel(coord + dxy.xy)+texel(coord - dxy.xy)+texel(coord - dxy.zy))*kern_d;
				return c;
			}
		)";
	} else {
		codes += R"(
				vec3 get_texel(const in vec2 coord) {
				return texel(coord);
			}
		)";
	}
	codes += R"(
		vec3 get_color(const in vec2 coord) {
			vec3 tex = get_texel(coord);
			tex -= orig_vec;
			tex *= conv_mat;
			tex += conv_vec;
			return tex;
		}
	)";

	codes += R"(
			void main() {
				vec3 c = get_color(qt_TexCoord);
				gl_FragColor.xyz = c;
				gl_FragColor.w = 1.0;
			}
	)";
	return codes;
}

void TextureShader::upload(const VideoFrame &frame) {
	Q_ASSERT(m_uploader);
	for (int i=0; i<m_info.size(); ++i)
		m_uploader->upload(m_info[i], frame);
}

void TextureShader::updateMatrix() {
	auto color = m_color;
	if (m_effects & VideoRendererItem::Grayscale)
		color.setSaturation(-1.0);
	auto mat = color.matrix(m_uploader->colorspace(m_format), m_uploader->colorrange(m_format));
	m_conv_mat = mat.transposed().toGenericMatrix<3, 3>();
	m_conv_vec = mat.column(3).toVector3D();
	m_orig_vec = mat.row(3).toVector3D();
	if (m_effects & VideoRendererItem::InvertColor) {
		m_conv_mat *= -1;
		m_conv_vec += QVector3D(1, 1, 1);
	}
}


void TextureShader::link(QOpenGLShaderProgram *program) {
	loc_kern_c = program->uniformLocation("kern_c");
	loc_kern_d = program->uniformLocation("kern_d");
	loc_kern_n = program->uniformLocation("kern_n");
	loc_p1 = program->uniformLocation("p1");
	loc_p2 = program->uniformLocation("p2");
	loc_p3 = program->uniformLocation("p3");
	loc_sc1 = program->uniformLocation("sc1");
	loc_sc2 = program->uniformLocation("sc2");
	loc_sc3 = program->uniformLocation("sc3");
	loc_conv_mat = program->uniformLocation("conv_mat");
	loc_conv_vec = program->uniformLocation("conv_vec");
	loc_orig_vec = program->uniformLocation("orig_vec");
}

bool TextureShader::setEffects(int effects) {
	constexpr auto kernel = VideoRendererItem::KernelEffects;
	const bool ret = ((effects & kernel) == (m_effects & kernel));
	m_effects = effects;
	updateMatrix();
	return ret;
}

void TextureShader::render(QOpenGLShaderProgram *program, const Kernel3x3 &kernel) {
	program->setUniformValue(loc_p1, 0);
	program->setUniformValue(loc_p2, 1);
	program->setUniformValue(loc_p3, 2);
	program->setUniformValue(loc_conv_mat, m_conv_mat);
	program->setUniformValue(loc_conv_vec, m_conv_vec);
	program->setUniformValue(loc_orig_vec, m_orig_vec);
	program->setUniformValue(loc_sc1, m_sc1);
	program->setUniformValue(loc_sc2, m_sc2);
	program->setUniformValue(loc_sc3, m_sc3);
	if (VideoRendererItem::KernelEffects & m_effects) {
		program->setUniformValue(loc_kern_c, kernel.center());
		program->setUniformValue(loc_kern_n, kernel.neighbor());
		program->setUniformValue(loc_kern_d, kernel.diagonal());
	}
	if (!format().isEmpty()) {
		auto f = QOpenGLContext::currentContext()->functions();
		f->glActiveTexture(GL_TEXTURE0);
		glBindTexture(m_target, m_textures[0]);
		f->glActiveTexture(GL_TEXTURE1);
		glBindTexture(m_target, m_textures[1]);
		f->glActiveTexture(GL_TEXTURE2);
		glBindTexture(m_target, m_textures[2]);
		f->glActiveTexture(GL_TEXTURE0);
	}
}

/****************************************************************************/

struct BlackOutShader : public TextureShader {
	BlackOutShader(const VideoFormat &format, GLenum target)
	: TextureShader(format, target) {
		setTexel(R"(vec3 texel(const in vec2 coord) { return vec3(0.0, 0.0, 0.0); })");
	}
};

/****************************************************************************/

struct I420Shader : public TextureShader {
	I420Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: TextureShader(format, target) {
		setTexel(R"(
			vec3 texel(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).x;
				yuv.y = texture2(coord).x;
				yuv.z = texture3(coord).x;
				return yuv;
			}
		)");
		auto init = [this, &format] (int i) {
			addTexInfo(i, format.bytesPerLine(i), format.lines(i), GL_LUMINANCE);
			sc(i).rx() *= (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*2);
		};
		init(0); init(1); init(2);
		if (target == GL_TEXTURE_RECTANGLE_ARB) { sc(1) *= 0.5; sc(2) *= 0.5; }\
	}
};

/****************************************************************************/

struct NvShader : public TextureShader {
	NvShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: TextureShader(format, target) {
		QByteArray texel = (R"(
			vec3 texel(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).x;
				yuv.yz = texture2(coord).!!;
				return yuv;
			}
		)");
		if (format.type() == IMGFMT_NV12)
			texel.replace("!!", "xw");
		else
			texel.replace("!!", "wx");
		setTexel(texel);
		addTexInfo(0, format.bytesPerLine(0), format.lines(0), GL_LUMINANCE);
		addTexInfo(1, format.bytesPerLine(1)/2, format.lines(1), GL_LUMINANCE_ALPHA);
		sc(1).rx() *= (double)format.bytesPerLine(0)/(double)format.bytesPerLine(1);
		if (target == GL_TEXTURE_RECTANGLE_ARB) { sc(1) *= 0.5; }
	}
};

/****************************************************************************/

struct Y422Shader : public TextureShader {
	Y422Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: TextureShader(format, target) {
		QByteArray texel = (R"(
			vec3 texel(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).?;
				yuv.yz = texture2(coord).!!;
				return yuv;
			}
		)");
		if (format.type() == IMGFMT_YUYV)
			texel.replace("?", "x").replace("!!", "yw");
		else
			texel.replace("?", "a").replace("!!", "zx");
		setTexel(texel);
		addTexInfo(0, format.bytesPerLine(0)/2, format.lines(0), GL_LUMINANCE_ALPHA);
		addRgbInfo(0, format.bytesPerLine(0)/4, format.lines(0), GL_BGRA);
		if (target == GL_TEXTURE_RECTANGLE_ARB)
			sc(1).rx() *= 0.5;
	}
};

/**************************************************************************/

struct PassThroughShader : public TextureShader {
	PassThroughShader(const VideoFormat &format, GLenum target): TextureShader(format, target) {}
};

/****************************************************************************/

struct RgbShader : public PassThroughShader {
	RgbShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: PassThroughShader(format, target) {
		GLenum glfmt = GL_BGRA;
		if (format.type() == IMGFMT_RGBA)
			glfmt = GL_RGBA;
		addRgbInfo(0, format.bytesPerLine(0)/4, format.lines(0), glfmt);
	}
};

/****************************************************************************/

#ifdef Q_OS_MAC

#include <OpenGL/CGLIOSurface.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <qpa/qplatformnativeinterface.h>


struct AppleY422Shader : public PassThroughShader {
	AppleY422Shader(const VideoFormat &format, GLenum target): PassThroughShader(format, target) {
		GLenum type = GL_UNSIGNED_SHORT_8_8_APPLE;
		if (format.type() == IMGFMT_YUYV)
			type = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
		addTexInfo(0, format.width(), format.height(), GL_YCBCR_422_APPLE, type, GL_RGB);
	}
};

struct VdaUploader : public TextureUploader {
	virtual void upload(const TextureInfo &info, const VideoFrame &frame) override {
		const auto m_cgl = static_cast<CGLContextObj>(qApp->platformNativeInterface()->nativeResourceForContext("cglcontextobj", QOpenGLContext::currentContext()));
		const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)frame.data(3));
		glBindTexture(info.target, info.id);
		CGLTexImageIOSurface2D(m_cgl, info.target, info.internal, info.width, info.height, info.format, info.type, surface, info.plane);
	}
	virtual void initialize(const TextureInfo &/*info*/) override {}
	virtual QImage toImage(const VideoFrame &frame) const override {
		mp_image mpi; memset(&mpi, 0, sizeof(mpi));
		mp_image_setfmt(&mpi, frame.format().type());
		mp_image_set_size(&mpi, frame.width(), frame.height());
		const auto buffer = (CVPixelBufferRef)frame.data(3);
		CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
		if (CVPixelBufferIsPlanar(buffer)) {
			const int num = CVPixelBufferGetPlaneCount(buffer);
			for (int i=0; i<num; ++i) {
				mpi.planes[i] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, i);
				mpi.stride[i] = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
			}
		} else {
			mpi.planes[0] = (uchar*)CVPixelBufferGetBaseAddress(buffer);
			mpi.stride[0] = CVPixelBufferGetBytesPerRow(buffer);
		}
		const QImage image = VideoFrame(&mpi, frame.format()).toImage();
		CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
		return image;
	}
};

#endif

#ifdef Q_OS_LINUX

#include "hwacc_vaapi.hpp"
#include <va/va_glx.h>

struct MesaY422Shader : public TextureShader {
	MesaY422Shader(const VideoFormat &format, GLenum target): TextureShader(format, target) {
		QByteArray getter(R"(vec3 texel(const vec2 coord) { return texture1(coord).y!!; })");
		GLenum type = GL_UNSIGNED_SHORT_8_8_MESA;
		if (format.type() == IMGFMT_YUYV) {
			type = GL_UNSIGNED_SHORT_8_8_REV_MESA;
			getter.replace("!!", "zx");
		} else
			getter.replace("!!", "xz");
		setTexel(getter);
		addTexInfo(0, format.width(), format.height(), GL_YCBCR_MESA, type, GL_YCBCR_MESA);
	}
};

struct VaApiUploader : public TextureUploader {
	static const int specs[MP_CSP_COUNT];
	virtual void initialize(const TextureInfo &info) override {
		free();
		Q_ASSERT(info.format == GL_BGRA);
		TextureUploader::initialize(info);
		vaCreateSurfaceGLX(VaApi::glx(), info.target, m_texture = info.id, &m_surface);
	}
	~VaApiUploader() { free(); }
	virtual void upload(const TextureInfo &info, const VideoFrame &frame) override {
		Q_ASSERT(m_texture == info.id);
		glBindTexture(info.target, info.id);
		const auto id = (VASurfaceID)(uintptr_t)frame.data(3);
		vaSyncSurface(VaApi::glx(), id);
		vaCopySurfaceGLX(VaApi::glx(), m_surface, id, specs[frame.format().colorspace()]);
	}
	virtual QImage toImage(const VideoFrame &frame) const override {
		QImage image(frame.format().alignedSize(), QImage::Format_ARGB32);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
		return image;
	}
	virtual mp_csp colorspace(const VideoFormat &) const override { return MP_CSP_RGB; }
private:
	void free() {
		if (m_surface)
			vaDestroySurfaceGLX(VaApi::glx(), m_surface);
		m_surface = nullptr;
	}

	void *m_surface = nullptr;
	GLuint m_texture = GL_NONE;
};

const int VaApiUploader::specs[MP_CSP_COUNT] = {
	0,					//MP_CSP_AUTO,
	VA_SRC_BT601,		//MP_CSP_BT_601,
	VA_SRC_BT709,		//MP_CSP_BT_709,
	VA_SRC_SMPTE_240,	//MP_CSP_SMPTE_240M,
	0,					//MP_CSP_RGB,
	0,					//MP_CSP_XYZ,
	0,					//MP_CSP_YCGCO,
};

#endif

TextureShader *TextureShader::create(const VideoFormat &format, const ColorProperty &color, int effects) {
	TextureShader *shader = nullptr;
	auto target = GL_TEXTURE_2D;
#ifdef Q_OS_MAC
	if (format.isNative())
		target = GL_TEXTURE_RECTANGLE_ARB;
#endif
#define MAKE(name) {shader = new name(format, target); break;}
	switch (format.type()) {
	case IMGFMT_420P:
		MAKE(I420Shader)
	case IMGFMT_NV21:
	case IMGFMT_NV12:
		MAKE(NvShader)
	case IMGFMT_YUYV:
	case IMGFMT_UYVY:
#ifdef Q_OS_MAC
		MAKE(AppleY422Shader)
#else
		if (QOpenGLContext::currentContext()->hasExtension("GL_MESA_ycbcr_texture")) {
			qDebug() << "Good! We have GL_MESA_ycbcr_texture.";
			MAKE(MesaY422Shader)
		} else
#endif
		MAKE(Y422Shader)
	case IMGFMT_BGRA:
	case IMGFMT_RGBA:
		MAKE(RgbShader)
	default:
		MAKE(BlackOutShader)
	}
	if (format.isNative())
#ifdef Q_OS_MAC
		shader->setUploader(new VdaUploader);
#endif
#ifdef Q_OS_LINUX
		shader->setUploader(new VaApiUploader);
#endif
	else
		shader->setUploader(new TextureUploader);
	shader->m_effects = effects;
	shader->m_color = color;
	shader->updateMatrix();
	return shader;
}
