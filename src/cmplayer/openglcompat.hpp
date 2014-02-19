#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "openglmisc.hpp"

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
	static OpenGLTextureTransferInfo textureTransferInfo(OGL::TransferFormat format, int bytesPerComponent = 1);
	static QByteArray rg(const char *rg) { return  hasExtension(TextureRG) ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return m_maxTexSize; }
	static bool hasExtension(Extension ext) { return m_extensions & ext; }
	static const OpenGLCompat &get() { return c; }
	static QOpenGLFunctions *functions() {
		auto ctx = QOpenGLContext::currentContext();
		return ctx ? ctx->functions() : nullptr;
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
	static OGL::TextureFormat framebufferObjectTextureFormat();
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
