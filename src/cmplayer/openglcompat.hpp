#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "openglmisc.hpp"

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define GL_UNSIGNED_SHORT_8_8_MESA      0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA  0x85BB
#endif

class OpenGLCompat {
public:
	static void initialize(QOpenGLContext *ctx);
	static void finalize(QOpenGLContext *ctx);
	static OpenGLTextureFormat textureFormat(GLenum format, int bpc = 1);
	static QByteArray rg(const char *rg) { return HasRG ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return MaxTexSize; }
	static bool hasRG() { return HasRG; }
	static bool hasFloat() { return HasFloat; }
	static bool hasDebug() { return HasDebug; }
	static const OpenGLCompat &get() { return c; }
	static OpenGLTexture allocateDitheringTexture(GLuint id, Dithering type);
	static QOpenGLFunctions *functions() {
		auto ctx = QOpenGLContext::currentContext();
		return ctx ? ctx->functions() : nullptr;
	}
	static OpenGLTexture makeTexture(int width, int height, GLenum format, GLenum target = GL_TEXTURE_2D) {
		OpenGLTexture texture;
		texture.width = width; texture.height = height;
		texture.target = target;
		texture.format = textureFormat(format);
		texture.generate();
		texture.allocate();
		return texture;
	}
	~OpenGLCompat();
	static void logError(const char *at) {
		const auto e = glGetError();
		if (e != GL_NO_ERROR)
			qWarning("OpenGL error: %s(0x%x) at %s", OpenGLCompat::errorString(e), e, at);
	}
	static void check();
	static const char *errorString(GLenum error);
	static QOpenGLDebugLogger *logger();
	static QOpenGLTexture::TextureFormat framebufferObjectTextureFormat();
	static void debug(const QOpenGLDebugMessage &message);
private:
	OpenGLCompat();
	static OpenGLCompat c;
	struct Data;
	Data *d;
	static bool HasRG, HasFloat, HasFbo, HasDebug;
	static int MaxTexSize;
};

#ifdef CMPLAYER_RELEASE
#define LOG_GL_ERROR
#define LOG_GL_ERROR_Q
#else
#define LOG_GL_ERROR {\
	auto e = glGetError(); \
		if (e != GL_NO_ERROR) \
			qWarning("OpenGL error: %s(0x%x) at %s line %d in %s", \
				OpenGLCompat::errorString(e), e, __FILE__, __LINE__, __PRETTY_FUNCTION__);\
	}

#define LOG_GL_ERROR_Q {\
	auto e = glGetError(); \
		if (e != GL_NO_ERROR) \
			qWarning("OpenGL error: %s(0x%x) at %s line %d in %s::%s", \
				OpenGLCompat::errorString(e), e, __FILE__, __LINE__, metaObject()->className(), __func__);\
	}
#endif

#endif // OPENGLCOMPAT_HPP
