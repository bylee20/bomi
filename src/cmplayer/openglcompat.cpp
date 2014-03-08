#include "openglcompat.hpp"
#include "videocolor.hpp"
#include "app.hpp"
#include <cmath>
#include "log.hpp"
#ifdef Q_OS_LINUX
#include <GL/glx.h>
#endif

DECLARE_LOG_CONTEXT(OpenGL)

OpenGLCompat OpenGLCompat::c;
int OpenGLCompat::m_maxTexSize = 0;
int OpenGLCompat::m_extensions = 0;

struct OpenGLCompat::Data {
	bool init = false;
	QOpenGLDebugLogger *logger = nullptr;
	int major = 0, minor = 0;
	QMap<OGL::TransferFormat, OpenGLTextureTransferInfo> formats[2];
	OGL::TextureFormat fboFormat = OGL::RGBA8_UNorm;
#ifdef Q_OS_LINUX
	int (*swapInterval)(int interval) = nullptr; // GLX_MESA, GLX_SGI, WGL_EXT
	void (*swapIntervalExt)(Display *dpy, GLXDrawable drawable, int interval) = nullptr; // GLX_EXT
#endif
};

OpenGLCompat::OpenGLCompat()
: d(new Data) {
}

OpenGLCompat::~OpenGLCompat() {
	delete d->logger;
	delete d;
}

QOpenGLDebugLogger *OpenGLCompat::logger() {
	return c.d->logger;
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Source source) {
	switch (source) {
#define SWITCH_SOURCE(s) case QOpenGLDebugMessage::s##Source: return #s;
	SWITCH_SOURCE(API)				SWITCH_SOURCE(Invalid)
	SWITCH_SOURCE(WindowSystem)		SWITCH_SOURCE(ShaderCompiler)
	SWITCH_SOURCE(ThirdParty)		SWITCH_SOURCE(Application)
	SWITCH_SOURCE(Other)			SWITCH_SOURCE(Any)
#undef SWITCH_SOURCE
	}
	return QByteArray::number(source, 16);
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Type type) {
	switch (type) {
#define SWITCH_TYPE(t) case QOpenGLDebugMessage::t##Type: return #t;
	SWITCH_TYPE(Invalid)			SWITCH_TYPE(Error)
	SWITCH_TYPE(DeprecatedBehavior)	SWITCH_TYPE(UndefinedBehavior)
	SWITCH_TYPE(Portability)		SWITCH_TYPE(Performance)
	SWITCH_TYPE(Other)				SWITCH_TYPE(Marker)
	SWITCH_TYPE(GroupPush)			SWITCH_TYPE(GroupPop)
	SWITCH_TYPE(Any)
#undef SWITCH_TYPE
	}
	return QByteArray::number(type, 16);
}

static inline QByteArray _ToLog(QOpenGLDebugMessage::Severity severity) {
	switch (severity) {
#define SWITCH_SEVERITY(s) case QOpenGLDebugMessage::s##Severity: return #s;
	SWITCH_SEVERITY(Invalid)		SWITCH_SEVERITY(High)
	SWITCH_SEVERITY(Medium)			SWITCH_SEVERITY(Low)
	SWITCH_SEVERITY(Notification)	SWITCH_SEVERITY(Any)
#undef SWITCH_SEVERITY
	}
	return QByteArray::number(severity, 16);
}

void OpenGLCompat::debug(const QOpenGLDebugMessage &message) {
	if (message.type() == QOpenGLDebugMessage::ErrorType)
		_Error("Error: %%", message.message().trimmed());
	else
		_Debug("Logger: %% (%%/%%/%%)", message.message().trimmed(), message.source(), message.severity(), message.type());
}

const char *OpenGLCompat::errorString(GLenum error) {
	static QHash<GLenum, const char*> strings;
	if (strings.isEmpty()) {
#define ADD(e) {strings[e] = #e;}
		ADD(GL_NO_ERROR);
		ADD(GL_INVALID_ENUM);
		ADD(GL_INVALID_VALUE);
		ADD(GL_INVALID_OPERATION);
		ADD(GL_STACK_OVERFLOW);
		ADD(GL_STACK_UNDERFLOW);
		ADD(GL_OUT_OF_MEMORY);
		ADD(GL_INVALID_FRAMEBUFFER_OPERATION);
		ADD(GL_TABLE_TOO_LARGE);
#undef ADD
	}
	return strings.value(error, "");
}

OGL::TextureFormat OpenGLCompat::framebufferObjectTextureFormat() {
	return c.d->fboFormat;
}

void OpenGLCompat::check() {
	QOpenGLContext gl;
	if (!gl.create())
		_Fatal("Cannot create OpenGL context!");
	QOffscreenSurface off;
	off.setFormat(gl.format());
	off.create();
	if (!gl.makeCurrent(&off))
		_Fatal("Cannot make OpenGL context current!");

	auto d = c.d;

	_Info("Check OpenGL stuffs.");
	const auto version = QOpenGLVersionProfile(gl.format()).version();
	d->major = version.first;
	d->minor = version.second;
	_Info("Version: %%.%%", d->major, d->minor);
	auto versionNumber = [] (int major, int minor) { return major*100 + minor; };
	const int current = versionNumber(version.first, version.second);
	if (current < versionNumber(2, 1))
		_Fatal("OpenGL version is too low. CMPlayer requires OpenGL 2.1 or higher.");

	auto exts = gl.extensions();
#ifdef Q_OS_LINUX
	exts += QSet<QByteArray>::fromList(QByteArray(glXQueryExtensionsString(QX11Info::display(), QX11Info::appScreen())).split(' '));
#endif
	QStringList extensions;
	auto checkExtension = [&] (const char *name, Extension ext, int major = -1, int minor = 0) {
		if ((major > 0 && current >= versionNumber(major, minor)) || exts.contains(name)) {
			extensions.append(name);
			m_extensions |= ext;
		}
	};

	checkExtension("GL_ARB_texture_rg", TextureRG, 3);
	checkExtension("GL_ARB_texture_float", TextureFloat, 3);
	checkExtension("GL_KHR_debug", Debug);
	checkExtension("GL_NV_vdpau_interop", NvVdpauInterop);
	checkExtension("GL_APPLE_ycbcr_422", AppleYCbCr422);
	checkExtension("GL_MESA_ycbcr_texture", MesaYCbCrTexture);
	checkExtension("GLX_EXT_swap_control", ExtSwapControl);
	checkExtension("GLX_SGI_swap_control", SgiSwapControl);
	checkExtension("GLX_MESA_swap_control", MesaSwapControl);

	if (QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()) {
		extensions.append("GL_ARB_framebuffer_object");
		m_extensions |= FramebufferObject;
	}
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTexSize);

	d->formats[0][OGL::Red] = {OGL::R8_UNorm, OGL::Red, OGL::UInt8};
	d->formats[0][OGL::RG] = {OGL::RG8_UNorm, OGL::RG, OGL::UInt8};
	d->formats[0][OGL::Luminance] = {OGL::Luminance8_UNorm, OGL::Luminance, OGL::UInt8};
	d->formats[0][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha8_UNorm, OGL::LuminanceAlpha, OGL::UInt8};
	d->formats[0][OGL::RGB] = {OGL::RGB8_UNorm, OGL::RGB, OGL::UInt8};
	d->formats[0][OGL::BGR] = {OGL::RGB8_UNorm, OGL::BGR, OGL::UInt8};
	d->formats[0][OGL::BGRA] = {OGL::RGBA8_UNorm, OGL::BGRA, OGL::UInt32_8_8_8_8_Rev};
	d->formats[0][OGL::RGBA] = {OGL::RGBA8_UNorm, OGL::RGBA, OGL::UInt32_8_8_8_8_Rev};

	d->formats[1][OGL::Red] = {OGL::R16_UNorm, OGL::Red, OGL::UInt16};
	d->formats[1][OGL::RG] = {OGL::R16_UNorm, OGL::RG, OGL::UInt16};
	d->formats[1][OGL::Luminance] = {OGL::Luminance16_UNorm, OGL::Luminance, OGL::UInt16};
	d->formats[1][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha16_UNorm, OGL::LuminanceAlpha, OGL::UInt16};
	d->formats[1][OGL::RGB] = {OGL::RGB16_UNorm, OGL::RGB, OGL::UInt16};
	d->formats[1][OGL::BGR] = {OGL::RGB16_UNorm, OGL::BGR, OGL::UInt16};
	d->formats[1][OGL::BGRA] = {OGL::RGBA16_UNorm, OGL::BGRA, OGL::UInt16};
	d->formats[1][OGL::RGBA] = {OGL::RGBA16_UNorm, OGL::RGBA, OGL::UInt16};

	const bool rg = hasExtension(TextureRG);
	for (auto &format : d->formats) {
		format[OGL::OneComponent] = rg ? format[OGL::Red] : format[OGL::Luminance];
		format[OGL::TwoComponents] = rg ? format[OGL::RG]  : format[OGL::LuminanceAlpha];
	}

	if (!hasExtension(FramebufferObject))
		_Fatal("FBO is not available. FBO support is essential.");
	auto fbo = new OpenGLFramebufferObject(QSize(16, 16), OGL::RGBA16_UNorm);
	if (fbo->isValid()) {
		d->fboFormat = OGL::RGBA8_UNorm;
		_Info("FBO texture format: GL_RGBA16");
	} else {
		if (!_Renew(fbo, QSize(16, 16), OGL::RGBA8_UNorm)->isValid())
			_Fatal("No available FBO texture format. One of GL_BGRA8 and GL_BGRA16 must be supported at least.");
		else
			_Info("FBO texture format: OGL::RGBA8_UNorm");
	}
	_Delete(fbo);
	if (!extensions.isEmpty())
		_Info("Available extensions: %%", extensions.join(", "));
}

void OpenGLCompat::finalize(QOpenGLContext */*ctx*/) {
	auto d = c.d;
	if (d->init) {
		if (d->logger && d->logger->isLogging())
			d->logger->stopLogging();
		_Delete(d->logger);
		d->init = false;
	}
}

void OpenGLCompat::initialize(QOpenGLContext *ctx) {
	auto d = c.d;
	if (d->init)
		return;
	d->init = true;
	if (cApp.isOpenGLDebugLoggerRequested()) {
		if (hasExtension(Debug) && ctx->format().testOption(QSurfaceFormat::DebugContext)) {
			d->logger = new QOpenGLDebugLogger;
			if (!d->logger->initialize()) {
				logError("OpenGLCompat::initialize()");
				delete d->logger;
			} else
				_Debug("OpenGL debug logger is running.");
		} else
			_Error("OpenGL debug logger was requested but it is not supported.");
	}
#ifdef Q_OS_LINUX
	if (hasExtension(ExtSwapControl))
		d->swapIntervalExt = (decltype(d->swapIntervalExt))ctx->getProcAddress("glXSwapIntervalEXT");
	if (hasExtension(MesaSwapControl))
		d->swapInterval = (decltype(d->swapInterval))ctx->getProcAddress("glxSwapIntervalMESA");
	else if (hasExtension(SgiSwapControl))
		d->swapInterval = (decltype(d->swapInterval))ctx->getProcAddress("glXSwapIntervalSGI");
#endif
}

OpenGLTextureTransferInfo OpenGLCompat::textureTransferInfo(OGL::TransferFormat format, int bytesPerComponent) {
	Q_ASSERT(bytesPerComponent == 1 || bytesPerComponent == 2);
	return c.d->formats[bytesPerComponent-1][format];
}

void OpenGLCompat::setSwapInterval(int frames) {
	Q_UNUSED(frames);
#ifdef Q_OS_LINUX
	if (c.d->swapInterval)
		c.d->swapInterval(frames);
#endif
}
