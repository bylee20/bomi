#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include <QOpenGLContext>
#include <array>

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
	int width = 0, height = 0;
	OpenGLTextureFormat format;
	void bind() const { glBindTexture(target, id); }
	void allocate(int filter = GL_LINEAR, const void *data = nullptr) const {
		bind();
		glTexImage2D(target, 0, format.internal, width, height, 0, format.pixel, format.type, data);
		glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	void upload(const void *data) const { upload(0, 0, width, height, data); }
	void upload(int x, int y, int width, int height, const void *data) const {
		bind(); glTexSubImage2D(target, 0, x, y, width, height, format.pixel, format.type, data);
	}
};

class OpenGLCompat {
public:
	static void initialize(QOpenGLContext *ctx) { c.fill(ctx); }
	static OpenGLTextureFormat textureFormat(GLenum format) { return c.m_formats[format]; }
	static bool hasRG() { return c.m_hasRG; } // use alpha instead of g if this is false
	static QByteArray rg(const char *rg) { return c.m_hasRG ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return c.m_maxTextureSize; }
	static const OpenGLCompat &get() { return c; }
private:
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
};

#endif // OPENGLCOMPAT_HPP
