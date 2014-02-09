#include "openglcompat.hpp"
#include "videocolor.hpp"
#include <cmath>
extern "C" {
#include <video/out/dither.h>
}

OpenGLCompat OpenGLCompat::c;
bool OpenGLCompat::HasRG = false;
bool OpenGLCompat::HasFloat = false;
int OpenGLCompat::MaxTexSize = 0;
bool OpenGLCompat::HasFbo = false;
bool OpenGLCompat::HasDebug = false;

struct OpenGLCompat::Data {
	bool init = false;
	QOpenGLDebugLogger *logger = nullptr;
	QOpenGLVersionProfile profile;
	int major = 0, minor = 0;
	QMap<GLenum, OpenGLTextureFormat> formats[2];
	QVector<GLfloat> fruit;
	void fill(QOpenGLContext *ctx) {
		if (init)
			return;
		init = true;
		profile = QOpenGLVersionProfile{ctx->format()};
		const auto version = profile.version();
		major = version.first;
		minor = version.second;

		formats[0][GL_RED] = {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
		formats[0][GL_RG] = {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
		formats[0][GL_LUMINANCE] = {GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE};
		formats[0][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE};
		formats[0][GL_RGB] = {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
		formats[0][GL_BGR] = {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE};
		formats[0][GL_BGRA] = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV};
		formats[0][GL_RGBA] = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV};

		formats[1][GL_RED] = {GL_R16, GL_RED, GL_UNSIGNED_SHORT};
		formats[1][GL_RG] = {GL_RG16, GL_RG, GL_UNSIGNED_SHORT};
		formats[1][GL_LUMINANCE] = {GL_LUMINANCE16, GL_LUMINANCE, GL_UNSIGNED_SHORT};
		formats[1][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT};
		formats[1][GL_RGB] = {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT};
		formats[1][GL_BGR] = {GL_RGB16, GL_BGR, GL_UNSIGNED_SHORT};
		formats[1][GL_BGRA] = {GL_RGBA16, GL_BGRA, GL_UNSIGNED_SHORT};
		formats[1][GL_RGBA] = {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT};
		for (auto &format : formats) {
			format[1] = HasRG ? format[GL_RED] : format[GL_LUMINANCE];
			format[2] = HasRG ? format[GL_RG]  : format[GL_LUMINANCE_ALPHA];
			format[3] = format[GL_BGR];
			format[4] = format[GL_BGRA];
		}
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTexSize);
		logError("OpenGLCompat::Data::fill()");
		if (HasDebug && ctx->format().testOption(QSurfaceFormat::DebugContext)) {
			logger = new QOpenGLDebugLogger;
			const bool ok = logger->initialize();
			Q_ASSERT(ok);
		}
	}
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

void OpenGLCompat::check() {
	auto ctx = QOpenGLContext::currentContext();
	Q_ASSERT(ctx != nullptr);
	auto version = QOpenGLVersionProfile(ctx->format()).version();
	auto major = version.first;
	HasRG = major >= 3 || ctx->hasExtension("GL_ARB_texture_rg");
	HasFloat = major >= 3 || ctx->hasExtension("GL_ARB_texture_float");
	HasFbo = QOpenGLFramebufferObject::hasOpenGLFramebufferObjects();
	if (qgetenv("CMPLAYER_GL_DEBUG").toInt())
		HasDebug = ctx->hasExtension("GL_KHR_debug");
}

void OpenGLCompat::initialize(QOpenGLContext *ctx) {
	c.d->fill(ctx);
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
		texture.format.internal = HasRG ? GL_R16 : GL_LUMINANCE16;
		texture.format.pixel = HasRG ? GL_RED : GL_LUMINANCE;
		if (HasFloat) {
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
	bind();
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
	release();
}
