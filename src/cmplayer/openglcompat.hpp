#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

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
	void bind() const { glBindTexture(target, id); }
	void allocate(int filter = GL_LINEAR, const void *data = nullptr) const {
		allocate(filter, GL_CLAMP_TO_EDGE, data);
	}
	void allocate(int filter, int clamp, const void *data = nullptr) const {
		bind();
		switch (target) {
		case GL_TEXTURE_3D:
			glTexImage3D(target, 0, format.internal, width, height, depth, 0, format.pixel, format.type, data);
			break;
		case GL_TEXTURE_2D:
			glTexImage2D(target, 0, format.internal, width, height, 0, format.pixel, format.type, data);
			break;
		case GL_TEXTURE_1D:
			glTexImage1D(target, 0, format.internal, width, 0, format.pixel, format.type, data);
			break;
		default:
			return;
		}
		glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filter);
		switch (target) {
		case GL_TEXTURE_3D:
			glTexParameterf(target, GL_TEXTURE_WRAP_R, clamp);
		case GL_TEXTURE_2D:
			glTexParameterf(target, GL_TEXTURE_WRAP_T, clamp);
		case GL_TEXTURE_1D:
			glTexParameterf(target, GL_TEXTURE_WRAP_S, clamp);
			break;
		default:
			return;
		}
	}
	void upload(const void *data) const {
		switch (target) {
		case GL_TEXTURE_3D:
			upload(0, 0, 0, width, height, depth, data);
			break;
		case GL_TEXTURE_2D:
			upload(0, 0, width, height, data);
			break;
		case GL_TEXTURE_1D:
			upload(0, width, data);
			break;
		default:
			return;
		}
	}
	void upload1D(const void *data) const { upload(0, width, data); }
	void upload2D(const void *data) const { upload(0, 0, width, height, data); }
	void upload(int x, int y, int z, int width, int height, int depth, const void *data) const {
		bind(); glTexSubImage3D(target, 0, x, y, z, width, height, depth, format.pixel, format.type, data);
	}
	void upload(int x, int y, int width, int height, const void *data) const {
		bind(); glTexSubImage2D(target, 0, x, y, width, height, format.pixel, format.type, data);
	}
	void upload(int x, int width, const void *data) const {
		bind(); glTexSubImage1D(target, 0, x, width, format.pixel, format.type, data);
	}
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

#endif // OPENGLCOMPAT_HPP
