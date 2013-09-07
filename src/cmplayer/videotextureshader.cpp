#include "videotextureshader.hpp"
#include "videoframe.hpp"
#include "hwacc.hpp"

VideoTextureShader::VideoTextureShader(const VideoFormat &format, GLenum target)
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
		uniform float top_field;
		uniform float deint;
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

void VideoTextureShader::initialize(GLuint *textures) {
	m_textures = textures;
	for (int i=0; i<m_info.size(); ++i) {
		m_info[i].id = textures[i];
		m_uploader->initialize(m_info[i]);
	}
}

QByteArray VideoTextureShader::fragment() const {
	QByteArray codes = m_header;
	codes += m_texel;
	QString dxy;
	const char *fmt = "const vec4 dxy = vec4(%f, %f, %f, %f);\n";
	if (m_target == GL_TEXTURE_2D && !format().isEmpty()) {
		const float dx = 1.0/(double)format().alignedWidth();
		const float dy = 1.0/(double)format().alignedHeight();
		dxy.sprintf(fmt, dx, dy, -dx, 0.f);
	} else
		dxy.sprintf(fmt, 1.f, 1.f, -1.f, 0.f);
	codes += dxy.toLatin1();
	if (m_effects & VideoRendererItem::KernelEffects) {
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
				vec3 get_texel(const in vec2 coord) { return texel(coord); }
		)";
	}
	QByteArray pp;
	if (m_deint.flags() & DeintInfo::OpenGL) {
		if (m_deint.method() == DeintInfo::Bob) {
			pp = R"(
				vec3 preprocess(const in vec2 coord) {
					float y = coord.y;
					float offset = deint*(top_field*dxy.y + mod(y, 2.0*dxy.y));
					return get_texel(coord - vec2(0.0, offset));
				}
			)";
		} else if (m_deint.method() == DeintInfo::LinearBob) {
			pp = R"(
				vec3 preprocess(const in vec2 coord) {
					float y = coord.y;
					float offset = deint*(top_field*dxy.y + mod(y, 2.0*dxy.y));
					return mix(get_texel(coord + vec2(0.0, -offset)), get_texel(coord + vec2(0.0, 2.0*dxy.y-offset)), offset/(2.0*dxy.y));
				}
			)";
		}
	}
	if (pp.isEmpty()) {
		pp = R"(
			vec3 preprocess(const in vec2 coord) { return get_texel(coord); }
		)";
	}
	codes += pp;
	codes += R"(
			void main() {
				vec3 tex = preprocess(qt_TexCoord);
				tex -= orig_vec;
				tex *= conv_mat;
				tex += conv_vec;
				const vec2 one = vec2(1.0, 0.0);
				gl_FragColor = tex.xyzz * one.xxxy + one.yyyx;
//				gl_FragColor = texel(qt_TexCoord).xxxx * one.xxxy + one.yyyx;
			}
	)";
	return codes;
}

void VideoTextureShader::upload(const VideoFrame &frame) {
	m_field = frame.field();
	Q_ASSERT(m_uploader);
	if (!(m_field & VideoFrame::Additional)) {
		for (int i=0; i<m_info.size(); ++i)
			m_uploader->upload(m_info[i], frame);
	}
}

void VideoTextureShader::updateMatrix() {
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


void VideoTextureShader::link(QOpenGLShaderProgram *program) {
	loc_kern_c = program->uniformLocation("kern_c");
	loc_kern_d = program->uniformLocation("kern_d");
	loc_kern_n = program->uniformLocation("kern_n");
	loc_top_field = program->uniformLocation("top_field");
	loc_deint = program->uniformLocation("deint");
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

bool VideoTextureShader::setEffects(int effects) {
	constexpr auto kernel = VideoRendererItem::KernelEffects;
	const bool ret = ((effects & kernel) == (m_effects & kernel));
	m_effects = effects;
	updateMatrix();
	return ret;
}

void VideoTextureShader::render(QOpenGLShaderProgram *program, const Kernel3x3 &kernel) {
	program->setUniformValue(loc_top_field, float(!!(m_field & VideoFrame::Top)));
	program->setUniformValue(loc_deint, float(!!(m_field & VideoFrame::Interlaced)));
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

struct BlackOutShader : public VideoTextureShader {
	BlackOutShader(const VideoFormat &format, GLenum target)
	: VideoTextureShader(format, target) {
		setTexel(R"(vec3 texel(const in vec2 coord) { return vec3(0.0, 0.0, 0.0); })");
	}
};

/****************************************************************************/

struct I420Shader : public VideoTextureShader {
	I420Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: VideoTextureShader(format, target) {
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
			if (format.imgfmt() != IMGFMT_VDA) // vda uploader takes the width not stride without correction
				sc(i).rx() *= (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*2);
		};
		init(0); init(1); init(2);
		if (target == GL_TEXTURE_RECTANGLE_ARB) { sc(1) *= 0.5; sc(2) *= 0.5; }\
	}
};

template<const int bit, const bool little>
struct P420BitShader : public VideoTextureShader { // planar 4:2:0 bit>8
	P420BitShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: VideoTextureShader(format, target) {
		QByteArray code = R"(
			 float convBits(const in vec4 tex) {
				 const vec2 c = vec2(265.0, 1.0)/(256.0*)";
		code += QByteArray::number((1 << (bit-8))-1);
		code += R"(.0/255.0 + 1.0);
				 return dot(tex.!!, c);
			 }
			 vec3 texel(const in vec2 coord) {
				vec3 yuv;
				 yuv.x = convBits(texture1(coord));
				 yuv.y = convBits(texture2(coord));
				 yuv.z = convBits(texture3(coord));
				return yuv;
			 }
		)";
		setTexel(code.replace("!!", little ? "wx" : "xw"));
		auto init = [this, &format] (int i) {
			addTexInfo(i, format.bytesPerLine(i)/2, format.lines(i), GL_LUMINANCE_ALPHA);
			sc(i).rx() *= (double)format.bytesPerLine(0)/(double)(format.bytesPerLine(i)*2);
		};
		init(0); init(1); init(2);
		if (target == GL_TEXTURE_RECTANGLE_ARB) { sc(1) *= 0.5; sc(2) *= 0.5; }\
	}
};

/****************************************************************************/

struct NvShader : public VideoTextureShader {
	NvShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: VideoTextureShader(format, target) {
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
		if (format.imgfmt() != IMGFMT_VDA) // vda uploader takes the width not stride without correction
			sc(1).rx() *= (double)format.bytesPerLine(0)/(double)format.bytesPerLine(1);
		if (target == GL_TEXTURE_RECTANGLE_ARB) { sc(1) *= 0.5; }
	}
};

/****************************************************************************/

struct Y422Shader : public VideoTextureShader {
	Y422Shader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D)
	: VideoTextureShader(format, target) {
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

struct PassThroughShader : public VideoTextureShader {
	PassThroughShader(const VideoFormat &format, GLenum target): VideoTextureShader(format, target) {}
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

struct VdaUploader : public VideoTextureUploader {
	virtual void upload(const VideoTextureInfo &info, const VideoFrame &frame) override {
		const auto m_cgl = static_cast<CGLContextObj>(qApp->platformNativeInterface()->nativeResourceForContext("cglcontextobj", QOpenGLContext::currentContext()));
		const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)frame.data(3));
		glBindTexture(info.target, info.id);
		const auto w = IOSurfaceGetWidthOfPlane(surface, info.plane);
		const auto h = IOSurfaceGetHeightOfPlane(surface, info.plane);
		CGLTexImageIOSurface2D(m_cgl, info.target, info.internal, w, h, info.format, info.type, surface, info.plane);
	}
	virtual void initialize(const VideoTextureInfo &/*info*/) override {}
	virtual QImage toImage(const VideoFrame &frame) const override {
		mp_image mpi = *nullMpImage();
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
	virtual mp_csp colorspace(const VideoFormat &format) const override {
		if (format.type() == IMGFMT_UYVY || format.type() == IMGFMT_YUYV)
			return MP_CSP_RGB;
		return VideoTextureUploader::colorspace(format);
	}
};

#endif

#ifdef Q_OS_LINUX

#include "hwacc_vaapi.hpp"
#include <va/va_glx.h>

struct MesaY422Shader : public VideoTextureShader {
	MesaY422Shader(const VideoFormat &format, GLenum target): VideoTextureShader(format, target) {
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

#include <va/va_x11.h>

struct VaApiDeinterlacer : public VaApiStatusChecker {
	VAContextID m_context = VA_INVALID_ID;
	VAConfigID m_config = VA_INVALID_ID;
	VAStatus m_status = VA_STATUS_SUCCESS;
	VABufferID m_deinterlacer = VA_INVALID_ID;
	QVector<VAProcColorStandardType> m_inputColors, m_outputColors;
	QVector<VASurfaceID> m_forwardReferencess, m_backwardReferences;
	VaApiDeinterlacer() {
		dpy = VaApi::glx();
		auto filter = VaApi::filter(VAProcFilterDeinterlacing);
		if (!filter || filter->algorithms().isEmpty())
			{isSuccess(VA_STATUS_ERROR_UNIMPLEMENTED); return;}
		if (!isSuccess(vaCreateConfig(dpy, VAProfileNone, VAEntrypointVideoProc, nullptr, 0, &m_config)))
			return;
		if (!isSuccess(vaCreateContext(dpy, m_config, 0, 0, 0, nullptr, 0, &m_context)))
			return;
		VAProcFilterParameterBufferDeinterlacing param;
		param.type = VAProcFilterDeinterlacing;
		param.algorithm = (VAProcDeinterlacingType)filter->algorithms().first();
		param.flags = 0;
		if (!isSuccess(vaCreateBuffer(dpy, m_context, VAProcFilterParameterBufferType, sizeof(param), 1, &param, &m_deinterlacer)))
			return;
		VAProcPipelineCaps caps;
		m_inputColors.resize(VAProcColorStandardCount);
		m_outputColors.resize(VAProcColorStandardCount);
		caps.input_color_standards = m_inputColors.data();
		caps.output_color_standards = m_outputColors.data();
		caps.num_input_color_standards = m_inputColors.size();
		caps.num_output_color_standards = m_outputColors.size();
		if (!isSuccess(vaQueryVideoProcPipelineCaps(dpy, m_context, &m_deinterlacer, 1, &caps)))
			return;
		m_inputColors.resize(caps.num_input_color_standards);
		m_outputColors.resize(caps.num_output_color_standards);
		m_forwardReferencess.resize(caps.num_forward_references);
		m_backwardReferences.resize(caps.num_backward_references);
	}
	~VaApiDeinterlacer() {
		if (m_deinterlacer != VA_INVALID_ID)
			vaDestroyBuffer(dpy, m_deinterlacer);
		if (m_context != VA_INVALID_ID)
			vaDestroyContext(dpy, m_context);
		if (m_config != VA_INVALID_ID)
			vaDestroyConfig(dpy, m_config);
	}
	bool apply(VASurfaceID input, VASurfaceID output, bool top) {
		if (!isSuccess(vaBeginPicture(dpy, m_context, output)))
			return false;
		VABufferID pipeline = VA_INVALID_ID;
		VAProcPipelineParameterBuffer *param = nullptr;
		if (!isSuccess(vaCreateBuffer(dpy, m_context, VAProcPipelineParameterBufferType, sizeof(*param), 1, nullptr, &pipeline)))
			return false;
		vaMapBuffer(dpy, pipeline, (void**)&param);
		param->surface = input;
		param->surface_region = nullptr;
		param->output_region = nullptr;
		param->output_background_color = 0;
		param->filter_flags = top ? VA_TOP_FIELD : VA_BOTTOM_FIELD;
		param->filters = &m_deinterlacer;
		param->num_filters = 1;
		vaUnmapBuffer(dpy, pipeline);
		param->forward_references = m_forwardReferencess.data();
		param->num_forward_references = m_forwardReferencess.size();
		param->backward_references = m_backwardReferences.data();
		param->num_backward_references = m_backwardReferences.size();
		vaRenderPicture(dpy, m_context, &pipeline, 1);
		vaEndPicture(dpy, m_context);
		return true;
	}
private:
	VADisplay dpy;
};

struct VaApiUploader : public VideoTextureUploader {
	static const int specs[MP_CSP_COUNT];
	VaApiUploader(const DeintInfo &deint): VideoTextureUploader(deint) {
		if (deint.flags() & DeintInfo::VaApi)
			m_deint = new VaApiDeinterlacer;
	}
	virtual void initialize(const VideoTextureInfo &info) override {
		free();
		Q_ASSERT(info.format == GL_BGRA);
		VideoTextureUploader::initialize(info);
		vaCreateSurfaceGLX(VaApi::glx(), info.target, m_texture = info.id, &m_surface);
		vaCreateSurfaces(VaApi::glx(), info.width, info.height, VaApi::surfaceFormat(), 1, &m_ppSurface);
	}
	~VaApiUploader() { free(); delete m_deint; }
	virtual void upload(const VideoTextureInfo &info, const VideoFrame &frame) override {
		auto dpy = VaApi::glx();
		Q_ASSERT(m_texture == info.id);
		auto id = (VASurfaceID)(uintptr_t)frame.data(3);
		vaSyncSurface(dpy, id);
		if ((frame.field() & VideoFrame::Interlaced) && m_deint) {
			if (m_deint->apply(id, m_ppSurface, frame.field() & VideoFrame::Top))
				id = m_ppSurface;
		}
		glBindTexture(info.target, info.id);
		vaCopySurfaceGLX(dpy, m_surface, id, specs[frame.format().colorspace()]);
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
		if (m_ppSurface != VA_INVALID_SURFACE) {
			vaDestroySurfaces(VaApi::glx(), &m_ppSurface, 1);
			m_ppSurface = VA_INVALID_SURFACE;
		}
	}
	void *m_surface = nullptr;
	GLuint m_texture = GL_NONE;
	VASurfaceID m_ppSurface = VA_INVALID_SURFACE;
	VaApiDeinterlacer *m_deint = nullptr;
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

VideoTextureShader *VideoTextureShader::create(const VideoFormat &format, const ColorProperty &color, const DeintInfo &deint, int effects) {
	VideoTextureShader *shader = nullptr;
	auto target = GL_TEXTURE_2D;
#ifdef Q_OS_MAC
	if (format.isNative())
		target = GL_TEXTURE_RECTANGLE_ARB;
#endif
#define MAKE(name) {shader = new name(format, target); break;}
	switch (format.type()) {
	case IMGFMT_420P:
		MAKE(I420Shader)
	case IMGFMT_420P16_LE:
		MAKE((P420BitShader<16, true>))
	case IMGFMT_420P16_BE:
		MAKE((P420BitShader<16, false>))
	case IMGFMT_420P14_LE:
		MAKE((P420BitShader<14, true>))
	case IMGFMT_420P14_BE:
		  MAKE((P420BitShader<14, false>))
	case IMGFMT_420P12_LE:
		  MAKE((P420BitShader<12, true>))
	case IMGFMT_420P12_BE:
		  MAKE((P420BitShader<12, false>))
	case IMGFMT_420P10_LE:
		  MAKE((P420BitShader<10, true>))
	case IMGFMT_420P10_BE:
		  MAKE((P420BitShader<10, false>))
	case IMGFMT_420P9_LE:
		  MAKE((P420BitShader<9, true>))
	case IMGFMT_420P9_BE:
		  MAKE((P420BitShader<9, false>))
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
		shader->setUploader(new VaApiUploader(deint));
#endif
	else
		shader->setUploader(new VideoTextureUploader);
	shader->m_effects = effects;
	shader->m_color = color;
	shader->m_deint = deint;
	shader->updateMatrix();
	return shader;
}
