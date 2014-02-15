#include "openglcompat.hpp"
#include "videocolor.hpp"
#include "app.hpp"
#include <cmath>
extern "C" {
#include <video/out/dither.h>
}
#include "log.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

OpenGLCompat OpenGLCompat::c;
int OpenGLCompat::m_maxTexSize = 0;
int OpenGLCompat::m_extensions = 0;

struct OpenGLCompat::Data {
	bool init = false;
	QOpenGLDebugLogger *logger = nullptr;
	int major = 0, minor = 0;
	QMap<GLenum, OpenGLTextureFormat> formats[2];
	QVector<GLfloat> fruit;
	QOpenGLTexture::TextureFormat fboFormat = QOpenGLTexture::RGBA8_UNorm;
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
#define SWITCH_SOURCE(s) case QOpenGLDebugMessage::s##Source: return _ByteArrayLiteral(#s);
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
#define SWITCH_TYPE(t) case QOpenGLDebugMessage::t##Type: return _ByteArrayLiteral(#t);
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
#define SWITCH_SEVERITY(s) case QOpenGLDebugMessage::s##Severity: return _ByteArrayLiteral(#s);
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

QOpenGLTexture::TextureFormat OpenGLCompat::framebufferObjectTextureFormat() {
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

	QStringList extensions;
	auto checkExtension = [&] (const char *name, Extension ext, int major = -1, int minor = 0) {
		if ((major > 0 && current >= versionNumber(major, minor)) || gl.hasExtension(name)) {
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
	if (QOpenGLFramebufferObject::hasOpenGLFramebufferObjects()) {
		extensions.append("GL_ARB_framebuffer_object");
		m_extensions |= FramebufferObject;
	}
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTexSize);

	d->formats[0][GL_RED] = {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
	d->formats[0][GL_RG] = {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
	d->formats[0][GL_LUMINANCE] = {GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE};
	d->formats[0][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE};
	d->formats[0][GL_RGB] = {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
	d->formats[0][GL_BGR] = {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE};
	d->formats[0][GL_BGRA] = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV};
	d->formats[0][GL_RGBA] = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV};

	d->formats[1][GL_RED] = {GL_R16, GL_RED, GL_UNSIGNED_SHORT};
	d->formats[1][GL_RG] = {GL_RG16, GL_RG, GL_UNSIGNED_SHORT};
	d->formats[1][GL_LUMINANCE] = {GL_LUMINANCE16, GL_LUMINANCE, GL_UNSIGNED_SHORT};
	d->formats[1][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT};
	d->formats[1][GL_RGB] = {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT};
	d->formats[1][GL_BGR] = {GL_RGB16, GL_BGR, GL_UNSIGNED_SHORT};
	d->formats[1][GL_BGRA] = {GL_RGBA16, GL_BGRA, GL_UNSIGNED_SHORT};
	d->formats[1][GL_RGBA] = {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT};

	const bool rg = hasExtension(TextureRG);
	for (auto &format : d->formats) {
		format[1] = rg ? format[GL_RED] : format[GL_LUMINANCE];
		format[2] = rg ? format[GL_RG]  : format[GL_LUMINANCE_ALPHA];
		format[3] = format[GL_BGR];
		format[4] = format[GL_BGRA];
	}

	if (!hasExtension(FramebufferObject))
		_Fatal("FBO is not available. FBO support is essential.");
	auto fbo = new OpenGLFramebufferObject(QSize(16, 16), QOpenGLTexture::RGBA16_UNorm);
	if (fbo->isValid()) {
		d->fboFormat = QOpenGLTexture::RGBA8_UNorm;
		_Info("FBO texture format: GL_RGBA16");
	} else {
		if (!_Renew(fbo, QSize(16, 16), QOpenGLTexture::RGBA8_UNorm)->isValid())
			_Fatal("No available FBO texture format. One of GL_BGRA8 and GL_BGRA16 must be supported at least.");
		else
			_Info("FBO texture format: GL_RGBA8");
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
}

OpenGLTextureFormat OpenGLCompat::textureFormat(GLenum format, int bpc) {
	Q_ASSERT(bpc == 1 || bpc == 2);
	return c.d->formats[bpc-1][format];
}

// copied from mpv's gl_video.c
OpenGLTexture OpenGLCompat::allocateDitheringTexture(GLuint id, Dithering type) {
	OpenGLTexture texture;
	texture.id = id;
	if (type == Dithering::None)
		return texture;
	const int sizeb = 6;
	int size = 0;
	QByteArray data;
	if (type == Dithering::Fruit) {
		size = 1 << 6;
		auto &fruit = c.d->fruit;
		if (fruit.size() != size*size) {
			fruit.resize(size*size);
			mp_make_fruit_dither_matrix(fruit.data(), sizeb);
		}
		const bool rg = hasExtension(TextureRG);
		texture.format.internal = rg ? GL_R16 : GL_LUMINANCE16;
		texture.format.pixel = rg ? GL_RED : GL_LUMINANCE;
		if (hasExtension(TextureFloat)) {
			texture.format.type = GL_FLOAT;
			data.resize(sizeof(GLfloat)*fruit.size());
			memcpy(data.data(), fruit.data(), data.size());
		} else {
			texture.format.type = GL_UNSIGNED_SHORT;
			data.resize(sizeof(GLushort)*fruit.size());
			auto p = (GLushort*)data.data();
			for (auto v : fruit)
				*p++ = v*_Max<GLushort>();
		}
	} else {
		size = 8;
		data.resize(size*size);
		mp_make_ordered_dither_matrix((uchar*)data.data(), size);
		texture.format = textureFormat(1);
	}
	texture.width = texture.height = size;
	texture.target = GL_TEXTURE_2D;
	//	 gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//	 gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	texture.allocate(GL_NEAREST, GL_REPEAT, data.data());
	return texture;
}

template<typename T>
static QImage getImage(const QSize &size, const OpenGLTextureFormat &format) {
	if (size.isEmpty())
		return QImage();
	QImage image(size, QImage::Format_ARGB32);
	QVector<T> data(size.width()*size.height()*4);
	auto src = data.data();
	glReadPixels(0, 0, size.width(), size.height(), format.pixel, format.type, src);
	uchar *dst = image.bits();
	const qreal r = qreal(_Max<uchar>())/qreal(_Max<T>());
	for (int i=0; i<size.width()*size.height()*4; ++i)
		*dst++ = qRound(qreal(*src++)*r);
	return image;
}

QImage OpenGLFramebufferObject::toImage() const {
	if (m_texture.isNull())
		return QImage();
	const bool wasBound = isBound();
	if (!wasBound)
		const_cast<OpenGLFramebufferObject*>(this)->bind();
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	switch (m_texture.format.type) {
	case GL_UNSIGNED_BYTE:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
		return getImage<uchar>(m_texture.size(), m_texture.format);
	case GL_UNSIGNED_SHORT:
		return getImage<GLushort>(m_texture.size(), m_texture.format);
	default:
		return QImage();
	}
	if (!wasBound)
		const_cast<OpenGLFramebufferObject*>(this)->release();
}
