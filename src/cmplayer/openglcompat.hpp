#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "openglmisc.hpp"

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define OGL::UInt16_8_8_MESA      0x85BA
#define OGL::UInt16_8_8_REV_MESA  0x85BB
#endif

class OpenGLCompat {
public:
	enum Extension {
		TextureRG = 1,
		TextureFloat = 2,
		Debug = 4,
		NvVdpauInterop = 8,
		FramebufferObject = 16,
		AppleYCbCr422 = 32,
		MesaYCbCrTexture = 64
	};
	static void initialize(QOpenGLContext *ctx);
	static void finalize(QOpenGLContext *ctx);
	static OpenGLTextureFormat textureFormat(GLenum format, int bpc = 1);
	static QByteArray rg(const char *rg) { return  hasExtension(TextureRG) ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return m_maxTexSize; }
	static bool hasExtension(Extension ext) { return m_extensions & ext; }
	static const OpenGLCompat &get() { return c; }
	static OpenGLTexture allocateDitheringTexture(GLuint id, Dithering type);
	static QOpenGLFunctions *functions() {
		auto ctx = QOpenGLContext::currentContext();
		return ctx ? ctx->functions() : nullptr;
	}
	static OpenGLTexture makeTexture(int width, int height, GLenum format, OGL::Target target = OGL::Target2D) {
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
	static int m_maxTexSize;
	static int m_extensions;
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
