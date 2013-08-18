#include "textureshader.hpp"
#include "videoframe.hpp"
#include "hwacc_vda.hpp"
#include "shadervar.h"

TextureShader::TextureShader(const VideoFormat &format, GLenum target)
: m_format(format), m_target(target), m_uploader(new TextureUploader) {
	m_info.reserve(3);
	m_variables = R"(
		uniform float brightness, contrast;
		uniform mat2 sat_hue;
		uniform vec3 rgb_c;
		uniform float rgb_0;
		uniform float y_tan, y_b;
		uniform vec4 dxy;
		uniform float kern_c, kern_n, kern_d;
		uniform float sc[3];
		varying highp vec2 qt_TexCoord;
	)";
	if (target == GL_TEXTURE_RECTANGLE_ARB) {
		m_variables += R"(
			uniform sampler2DRect p1, p2, p3;
			vec4 texture1(const in vec2 coord) { return texture2DRect(p1, coord); }
			vec4 texture2(const in vec2 coord) { return texture2DRect(p2, coord*vec2(sc[1], 1.0)); }
			vec4 texture3(const in vec2 coord) { return texture2DRect(p3, coord*vec2(sc[2], 1.0)); }
		)";
	} else {
		m_variables += R"(
			uniform sampler2D p1, p2, p3;
			vec4 texture1(const in vec2 coord) { return texture2D(p1, coord); }
			vec4 texture2(const in vec2 coord) { return texture2D(p2, coord*vec2(sc[1], 1.0)); }
			vec4 texture3(const in vec2 coord) { return texture2D(p3, coord*vec2(sc[2], 1.0)); }
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
//		auto &info = m_info[i];
//		initTex(i, info.width, info.height, info.format, info.type, info.internal);
	}
}

QByteArray TextureShader::fragment(const ShaderVar &var) const {
	QByteArray codes = m_variables;
	return codes += getMain(var);
}

void TextureShader::upload(const VideoFrame &frame) {
	Q_ASSERT(m_uploader);
	for (int i=0; i<m_info.size(); ++i)
		m_uploader->upload(m_info[i], frame);
}

void TextureShader::link(QOpenGLShaderProgram *program) {
	loc_brightness = program->uniformLocation("brightness");
	loc_contrast = program->uniformLocation("contrast");
	loc_sat_hue = program->uniformLocation("sat_hue");
	loc_rgb_c = program->uniformLocation("rgb_c");
	loc_rgb_0 = program->uniformLocation("rgb_0");
	loc_y_tan = program->uniformLocation("y_tan");
	loc_y_b = program->uniformLocation("y_b");
	loc_dxy = program->uniformLocation("dxy");
	loc_kern_c = program->uniformLocation("kern_c");
	loc_kern_d = program->uniformLocation("kern_d");
	loc_kern_n = program->uniformLocation("kern_n");
	loc_p1 = program->uniformLocation("p1");
	loc_p2 = program->uniformLocation("p2");
	loc_p3 = program->uniformLocation("p3");
	loc_sc = program->uniformLocation("sc");
}

void TextureShader::render(QOpenGLShaderProgram *program, const ShaderVar &var) {
	program->setUniformValue(loc_p1, 0);
	program->setUniformValue(loc_p2, 1);
	program->setUniformValue(loc_p3, 2);
	program->setUniformValue(loc_brightness, var.brightness);
	program->setUniformValue(loc_contrast, var.contrast);
	program->setUniformValue(loc_sat_hue, var.sat_hue);
	if (m_target == GL_TEXTURE_2D) {
		const float dx = 1.0/(double)format().alignedWidth();
		const float dy = 1.0/(double)format().alignedHeight();
		program->setUniformValue(loc_dxy, dx, dy, -dx, 0.f);
	} else
		program->setUniformValue(loc_dxy, 1.f, 1.f, -1.f, 0.f);
	program->setUniformValueArray(loc_sc, m_strideCorrection, 3, 1);
	const auto effects = var.effects();
	const bool filter = effects & VideoRendererItem::FilterEffects;
	const bool kernel = effects & VideoRendererItem::KernelEffects;
	if (filter || kernel) {
		program->setUniformValue(loc_rgb_c, var.rgb_c[0], var.rgb_c[1], var.rgb_c[2]);
		program->setUniformValue(loc_rgb_0, var.rgb_0);
	}
	if (kernel) {
		program->setUniformValue(loc_kern_c, var.kern_c);
		program->setUniformValue(loc_kern_n, var.kern_n);
		program->setUniformValue(loc_kern_d, var.kern_d);
	}
	if (effects & VideoRendererItem::RemapLuma) {
		const float y_tan = 1.0/var.m_luma.difference();
		program->setUniformValue(loc_y_tan, y_tan);
		program->setUniformValue(loc_y_b, (float)-var.m_luma.min);
	} else {
		program->setUniformValue(loc_y_tan, 1.0f);
		program->setUniformValue(loc_y_b, 0.0f);
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

class BlackOutShader : public TextureShader {
public:
	BlackOutShader(const VideoFormat &format, GLenum target): TextureShader(format, target) {}
	QByteArray getMain(const ShaderVar &/*var*/) const override {
		return R"(
			void main() {
				gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
			}
		)";
	}
};

/****************************************************************************/

class YCbCrShader : public TextureShader {
public:
	YCbCrShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: TextureShader(format, target) {
		m_funcs = R"(
			void convert(inout vec3 yuv) {
				const vec3 yuv_0 = vec3(0.0625, 0.5, 0.5);

				yuv -= yuv_0;

				yuv.yz *= sat_hue;
				yuv *= contrast;
				yuv.x += brightness;

				const mat3 coef = mat3(
					1.16438356,  0.0,          1.59602679,
					1.16438356, -0.391762290, -0.812967647,
					1.16438356,  2.01723214,   0.0
				);
				yuv *= coef;
			}

			void adjust_rgb(inout vec3 rgb) {
				rgb *= rgb_c;
				rgb += rgb_0;
			}

			void renormalize_y(inout float y) {
				y = y_tan*y + y_b;
			}

			void apply_filter_convert(inout vec3 yuv) {
				renormalize_y(yuv.x);
				convert(yuv);
				adjust_rgb(yuv);
			}

			vec3 get_yuv_kernel_applied(const in vec2 coord) {
				// dxy.zy   dxy.wy   dxy.xy
				// dxy.zw     0      dxy.xw
				//-dxy.xy  -dxy.wy  -dxy.zy
				vec3 c = get_yuv(coord)*kern_c;
				c += (get_yuv(coord + dxy.wy)+get_yuv(coord + dxy.zw)+get_yuv(coord + dxy.xw)+get_yuv(coord - dxy.wy))*kern_n;
				c += (get_yuv(coord + dxy.zy)+get_yuv(coord + dxy.xy)+get_yuv(coord - dxy.xy)+get_yuv(coord - dxy.zy))*kern_d;
				return c;
			}
		)";
	}

	QByteArray getMain(const ShaderVar &var) const override {
		static char SimpleMain[] = (R"(
			void main() {
				vec3 c = get_yuv(qt_TexCoord);
				convert(c);
				gl_FragColor.xyz = c;
				gl_FragColor.w = 1.0;
//				gl_FragColor.xyz = get_yuv(qt_TexCoord).xxx;
			}
		)");

		static char FilterMain[] = (R"(
		void main() {
			vec3 c = get_yuv(qt_TexCoord);
			apply_filter_convert(c);
			gl_FragColor.xyz = c;
			gl_FragColor.w = 1.0;
		}
		)");

		static char KernelMain[] = (R"(
		void main() {
			vec3 c = get_yuv_kernel_applied(qt_TexCoord);
			apply_filter_convert(c);
			gl_FragColor.xyz = c;
			gl_FragColor.w = 1.0;
		}
		)");

		auto ret = m_getter;
		ret += m_funcs;
		if (var.effects() & VideoRendererItem::KernelEffects)
			ret += KernelMain;
		else if (var.effects() & VideoRendererItem::FilterEffects)
			ret += FilterMain;
		else
			ret += SimpleMain;
		return ret;
	}
protected:
	void setGetter(const QByteArray &getter) { m_getter = getter; }
private:
	QByteArray m_funcs, m_getter;
};

/****************************************************************************/

class I420Shader : public YCbCrShader {
public:
	I420Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: YCbCrShader(format, target) {
		setGetter(R"(
			vec3 get_yuv(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).x;
				yuv.y = texture2(coord).x;
				yuv.z = texture3(coord).x;
				return yuv;
			}
		)");
		auto init = [this, &format] (int i) {
			addTexInfo(i, format.bytesPerLine(i), format.lines(i), GL_LUMINANCE);
			sc(i) = (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*2);
		};
		init(0); init(1); init(2);
	}
};

/****************************************************************************/

class NvShader : public YCbCrShader {
public:
	NvShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: YCbCrShader(format, target) {
		QByteArray get = (R"(
			vec3 get_yuv(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).x;
				yuv.yz = texture2(coord).!!;
				return yuv;
			}
		)");
		if (format.type() == IMGFMT_NV12)
			get.replace("!!", "xw");
		else
			get.replace("!!", "wx");
		setGetter(get);
		addTexInfo(0, format.bytesPerLine(0), format.lines(0), GL_LUMINANCE);
		addTexInfo(1, format.bytesPerLine(1)/2, format.lines(1), GL_LUMINANCE_ALPHA);
		sc(1) = (double)format.bytesPerLine(0)/(double)format.bytesPerLine(1);
	}
};

/****************************************************************************/

class Y422Shader : public YCbCrShader {
public:
	Y422Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: YCbCrShader(format, target) {
		QByteArray get = (R"(
			vec3 get_yuv(const vec2 coord) {
				vec3 yuv;
				yuv.x = texture1(coord).?;
				yuv.yz = texture2(coord).!!;
				return yuv;
			}
		)");
		if (format.type() == IMGFMT_YUYV)
			get.replace("?", "x").replace("!!", "yw");
		else
			get.replace("?", "a").replace("!!", "zx");
		setGetter(get);
		addTexInfo(0, format.bytesPerLine(0)/2, format.lines(0), GL_LUMINANCE_ALPHA);
		addRgbInfo(0, format.bytesPerLine(0)/4, format.lines(0), GL_BGRA);
		if (target == GL_TEXTURE_RECTANGLE_ARB)
			sc(1) = 0.5;
	}
};

/****************************************************************************/

class RgbShader : public TextureShader {
public:
	RgbShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: TextureShader(format, target) {
		GLenum glfmt = GL_BGRA;
		if (format.type() == IMGFMT_RGBA)
			glfmt = GL_RGBA;
		addRgbInfo(0, format.bytesPerLine(0)/4, format.lines(0), glfmt);
	}
	QByteArray getMain(const ShaderVar &/*var*/) const {
		return (R"(
			void main() { gl_FragColor = texture1(qt_TexCoord); }
		)");
	}
};

/****************************************************************************/


//#ifdef Q_OS_LINUX
//		case VideoFormat::VAGL:
//			glBindTexture(GL_TEXTURE_2D, texture(0));
//			if (!d->vaapi)
//				d->vaapi = new VaApiSurfaceGLX(texture(0));
//			d->vaapi->copy(d->frame.data(3));
//			if (d->take) {
//				QImage image(d->format.alignedSize(), QImage::Format_ARGB32);
//				glBindTexture(GL_TEXTURE_2D, texture(0));
//				glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
//				d->mposd->drawOn(image);
//				emit frameImageObtained(image);
//				d->take = false;
//			}
//			break;
//#endif
//		case VideoFormat::APGL:
//			if (!d->acc)
//				d->acc = new VdaSurface(textures());
//			d->acc->copy(d->frame.data(3));
//			break;
//		default:
//			break;
//		}
//	} else
//		setTex(0, GL_BGRA, d->image.width(), d->image.height(), d->image.bits());



#ifdef Q_OS_MAC

#include <OpenGL/CGLIOSurface.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <qpa/qplatformnativeinterface.h>

struct VdaUploader : public TextureUploader {
	virtual void upload(const TextureInfo &info, const VideoFrame &frame) override {
		const auto m_cgl = static_cast<CGLContextObj>(qApp->platformNativeInterface()->nativeResourceForContext("cglcontextobj", QOpenGLContext::currentContext()));
		const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)frame.data(3));
		glBindTexture(info.target, info.id);
		CGLTexImageIOSurface2D(m_cgl, info.target, info.internal, info.width, info.height, info.format, info.type, surface, info.plane);
	}
	virtual void initialize(const TextureInfo &/*info*/) override {}
};

#endif

#ifdef Q_OS_LINUX

#include "hwacc_vaapi.hpp"
#include <va/va_glx.h>

struct VaApiUploader : public TextureUploader {
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
		vaCopySurfaceGLX(VaApi::glx(), m_surface, id, VA_SRC_BT601);
	}
	virtual QImage toImage(const VideoFrame &frame) const override {
		QImage image(frame.format().alignedSize(), QImage::Format_ARGB32);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
		return image;
	}
private:
	void free() {
		if (m_surface)
			vaDestroySurfaceGLX(VaApi::glx(), m_surface);
		m_surface = nullptr;
	}

	void *m_surface = nullptr;
	GLuint m_texture = GL_NONE;
};

#endif

TextureShader *TextureShader::create(const VideoFormat &format) {
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
	return shader;
}
