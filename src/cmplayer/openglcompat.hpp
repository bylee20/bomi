#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include <QImage>
#include <QOpenGLContext>
#include <array>
#include "enums.hpp"

struct OpenGLTextureFormat {
	OpenGLTextureFormat() {}
	OpenGLTextureFormat(GLint internal, GLenum pixel, GLenum type)
	: internal(internal), pixel(pixel), type(type) {}
	GLint internal = GL_NONE; GLenum pixel = GL_NONE, type = GL_NONE;
};

struct OpenGLTexture {
	virtual ~OpenGLTexture() = default;
	GLuint id = GL_NONE;
	GLenum target = GL_TEXTURE_2D;
	int width = 0, height = 0, depth = 0;
	OpenGLTextureFormat format;
	void generate() { glGenTextures(1, &id); }
	void delete_() { glDeleteTextures(1, &id); }
	void bind() const { glBindTexture(target, id); }
	bool allocate(int filter = GL_LINEAR, const void *data = nullptr) const {
		return allocate(filter, GL_CLAMP_TO_EDGE, data);
	}
	bool allocate(int filter, int clamp, const void *data = nullptr) const {
		bind();
		if (!width && !height && !depth)
			return false;
		switch (target) {
		case GL_TEXTURE_3D:
			glTexImage3D(target, 0, format.internal, width, height, depth, 0, format.pixel, format.type, data);
			break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			glTexImage2D(target, 0, format.internal, width, height, 0, format.pixel, format.type, data);
			break;
		case GL_TEXTURE_1D:
			glTexImage1D(target, 0, format.internal, width, 0, format.pixel, format.type, data);
			break;
		default:
			return false;
		}
		glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filter);
		switch (target) {
		case GL_TEXTURE_3D:
			glTexParameterf(target, GL_TEXTURE_WRAP_R, clamp);
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			glTexParameterf(target, GL_TEXTURE_WRAP_T, clamp);
		case GL_TEXTURE_1D:
			glTexParameterf(target, GL_TEXTURE_WRAP_S, clamp);
			break;
		default:
			return false;
		}
		return true;
	}
	bool upload(const void *data) const {
		if (isEmpty())
			return false;
		switch (target) {
		case GL_TEXTURE_3D:
			upload(0, 0, 0, width, height, depth, data);
			break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			upload(0, 0, width, height, data);
			break;
		case GL_TEXTURE_1D:
			upload(0, width, data);
			break;
		default:
			return false;
		}
		return true;
	}
	bool isEmpty() const { return !width && !height && !depth; }
	bool upload1D(const void *data) const { return upload(0, width, data); }
	bool upload2D(const void *data) const { return upload(0, 0, width, height, data); }
	bool upload(int x, int y, int z, int width, int height, int depth, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage3D(target, 0, x, y, z, width, height, depth, format.pixel, format.type, data);
		return true;
	}
	bool upload(int x, int y, int width, int height, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage2D(target, 0, x, y, width, height, format.pixel, format.type, data);
		return true;
	}
	bool upload(int x, int width, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage1D(target, 0, x, width, format.pixel, format.type, data);
		return true;
	}
	void unbind() { glBindTexture(target, 0); }
};

class OpenGLCompat {
	static constexpr int CubicLutSamples = 256;
	static constexpr int CubicLutSize = CubicLutSamples*4;
public:
	static void initialize(QOpenGLContext *ctx) { c.fill(ctx); }
	static OpenGLTextureFormat textureFormat(GLenum format) { return c.m_formats[format]; }
	static bool hasRG() { return c.m_hasRG; } // use alpha instead of g if this is false
	static QByteArray rg(const char *rg) { return c.m_hasRG ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return c.m_maxTextureSize; }
	static const OpenGLCompat &get() { return c; }
	static OpenGLTexture allocateBicubicLutTexture(GLuint id, InterpolatorType type);
	static OpenGLTexture allocate3dLutTexture(GLuint id);
	static void upload3dLutTexture(const OpenGLTexture &texture, const QVector3D &sub, const QMatrix3x3 &mul, const QVector3D &add);
	static QOpenGLFunctions *functions() {
		auto ctx = QOpenGLContext::currentContext();
		return ctx ? ctx->functions() : nullptr;
	}
private:
	static QVector<GLushort> makeBicubicLut(double b, double c);
	void fill(QOpenGLContext *ctx);
	OpenGLCompat() = default;
	static OpenGLCompat c;
	bool m_init = false;
	GLenum m_oneInternal;
	GLenum m_twoInternal;
	QOpenGLVersionProfile m_profile;
	int m_major = 0, m_minor = 0;
	int m_maxTextureSize = 0;
	bool m_hasRG = false;
	QMap<GLenum, OpenGLTextureFormat> m_formats;
	std::array<QVector<GLushort>, InterpolatorTypeInfo::size()> m_bicubicLuts;
//	std::array<GLushort, CubicLutSize> m_bSplineLut;
	QVector<GLushort> m_3dLut;
	QVector3D m_subLut, m_addLut;
	QMatrix3x3 m_mulLut;
	std::array<QPair<double, double>, InterpolatorTypeInfo::size()> m_bicubicParams;
};

class OpenGLFramebufferObject {
public:
	OpenGLFramebufferObject(const QSize &size, int target = GL_TEXTURE_2D, int filter = GL_LINEAR) {
		auto f = func();
		f->glGenFramebuffers(1, &m_id);
		f->glBindFramebuffer(GL_FRAMEBUFFER, m_id);
		m_texture.generate();
		m_texture.width = size.width();
		m_texture.height = size.height();
		m_texture.target = target;
		m_texture.format = OpenGLCompat::textureFormat(GL_BGRA);
		m_texture.allocate(filter);
		f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texture.id, 0);
		QOpenGLFramebufferObject::bindDefault();
	}
	virtual ~OpenGLFramebufferObject() {
		m_texture.delete_();
		func()->glDeleteFramebuffers(1, &m_id);
	}
	int width() const { return m_texture.width; }
	int height() const { return m_texture.height; }
	QSize size() const { return {m_texture.width, m_texture.height}; }
	void bind() { func()->glBindFramebuffer(GL_FRAMEBUFFER, m_id); }
	void release() { QOpenGLFramebufferObject::bindDefault(); }
	const OpenGLTexture &texture() const { return m_texture; }
	QImage toImage() const {
		QImage image(m_texture.width, m_texture.height, QImage::Format_ARGB32);
		m_texture.bind();
		glGetTexImage(m_texture.target, 0, m_texture.format.pixel, m_texture.format.type, image.bits());
		return image;
	}
	void getCoords(double &x1, double &y1, double &x2, double &y2) {
		if (m_texture.target == GL_TEXTURE_RECTANGLE) {
			x1 = y1 = 0; x2 = m_texture.width; y2 = m_texture.height;
		} else { x1 = y1 = 0; x2 = y2 = 1; }
	}
private:
	static QOpenGLFunctions *func() { return OpenGLCompat::functions(); }
	GLuint m_id = GL_NONE;
	OpenGLTexture m_texture;
};

#endif // OPENGLCOMPAT_HPP
