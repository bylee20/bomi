#ifndef OPENGLCOMPAT_HPP
#define OPENGLCOMPAT_HPP

#include "stdafx.hpp"
#include "enums.hpp"

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define GL_UNSIGNED_SHORT_8_8_MESA      0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA  0x85BB
#endif

struct OpenGLTextureFormat {
	OpenGLTextureFormat() {}
	OpenGLTextureFormat(GLint internal, GLenum pixel, GLenum type)
	: internal(internal), pixel(pixel), type(type) {}
	GLint internal = GL_NONE; GLenum pixel = GL_NONE, type = GL_NONE;
};

class OpenGLTexture {
public:
	virtual ~OpenGLTexture() = default;
	GLuint id = GL_NONE;
	GLenum target = GL_TEXTURE_2D;
	int width = 0, height = 0, depth = 0;
	OpenGLTextureFormat format;
	QSize size() const { return {width, height}; }
	void setSize(const QSize &size) { width = size.width(); height = size.height(); }
	void generate() { glGenTextures(1, &id); }
	void delete_() { glDeleteTextures(1, &id); }
	void bind() const { glBindTexture(target, id); }
	bool allocate(int filter = GL_LINEAR, const void *data = nullptr) const {
		return allocate(filter, GL_CLAMP_TO_EDGE, data);
	}
	bool expand(const QSize &size, double mul = 1.2) {
		if (width >= size.width() && height >= size.height())
			return true;
		if (width < size.width())
			width = size.width()*mul;
		if (height < size.height())
			height = size.width()*mul;
		return allocate();
	}
	bool allocate(int filter, int clamp, const void *data = nullptr) const {
		if (!width && !height && !depth)
			return false;
		bind();
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
		unbind();
		return true;
	}
	bool upload(const void *data) const {
		switch (target) {
		case GL_TEXTURE_3D:
			return upload(0, 0, 0, width, height, depth, data);
		case GL_TEXTURE_2D:
		case GL_TEXTURE_RECTANGLE:
			return upload(0, 0, width, height, data);
		case GL_TEXTURE_1D:
			return upload(0, width, data);
		default:
			return false;
		}
	}
	bool isEmpty() const { return !width && !height && !depth; }
	bool upload1D(const void *data) const { return upload(0, width, data); }
	bool upload2D(const void *data) const { return upload(0, 0, width, height, data); }
	bool upload(int x, int y, int z, int width, int height, int depth, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage3D(target, 0, x, y, z, width, height, depth, format.pixel, format.type, data);
		unbind();
		return true;
	}
	bool upload(int x, int y, int width, int height, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage2D(target, 0, x, y, width, height, format.pixel, format.type, data);
		unbind();
		return true;
	}
	bool upload(int x, int y, const QSize &size, const void *data) const {
		return upload(x, y, size.width(), size.height(), data);
	}
	bool upload(const QPoint &pos, const QSize &size, const void *data) const {
		return upload(pos.x(), pos.y(), size.width(), size.height(), data);
	}
	bool upload(int x, int width, const void *data) const {
		if (isEmpty())
			return false;
		bind();
		glTexSubImage1D(target, 0, x, width, format.pixel, format.type, data);
		unbind();
		return true;
	}
	void unbind() const { glBindTexture(target, 0); }
};

class OpenGLCompat {
public:
	static constexpr int CubicLutSamples = 256;
	static constexpr int CubicLutSize = CubicLutSamples*4;
	static void initialize(QOpenGLContext *ctx) { c.fill(ctx); }
	static OpenGLTextureFormat textureFormat(GLenum format, int bpc = 1) {
		Q_ASSERT(bpc == 1 || bpc == 2); return c.m_formats[bpc-1][format];
	}
	static bool hasRG() { return c.m_hasRG; } // use alpha instead of g if this returns false
	static QByteArray rg(const char *rg) { return c.m_hasRG ? QByteArray(rg) : QByteArray(rg).replace('g', 'a'); }
	static int maximumTextureSize() { return c.m_maxTextureSize; }
	static const OpenGLCompat &get() { return c; }
	static OpenGLTexture allocateInterpolatorLutTexture(GLuint id, InterpolatorType type);
	static OpenGLTexture allocateDitheringTexture(GLuint id, Dithering type);
	static OpenGLTexture allocate3dLutTexture(GLuint id);
	static void upload3dLutTexture(const OpenGLTexture &texture, const QVector3D &sub, const QMatrix3x3 &mul, const QVector3D &add);
	static QOpenGLFunctions *functions() { auto ctx = QOpenGLContext::currentContext(); return ctx ? ctx->functions() : nullptr; }
	static OpenGLTexture makeTexture(int width, int height, GLenum format, GLenum target = GL_TEXTURE_2D, GLenum filter = GL_LINEAR) {
		OpenGLTexture texture;
		texture.width = width; texture.height = height;
		texture.target = target;
		texture.format = textureFormat(format);
		texture.generate();
		texture.allocate(filter);
		return texture;
	}
private:
	void fillInterpolatorLut(InterpolatorType type);
	void fill(QOpenGLContext *ctx);
	OpenGLCompat() = default;
	static OpenGLCompat c;
	bool m_init = false;
	QOpenGLVersionProfile m_profile;
	int m_major = 0, m_minor = 0;
	int m_maxTextureSize = 0;
	bool m_hasRG = false, m_hasFloat = false;
	QMap<GLenum, OpenGLTextureFormat> m_formats[2];
	std::array<QVector<GLushort>, InterpolatorTypeInfo::size()> m_intLuts;
	QVector<GLushort> m_3dLut;
	QVector<GLfloat> m_fruit;
	QVector3D m_subLut, m_addLut;
	QMatrix3x3 m_mulLut;
	std::array<QPair<double, double>, InterpolatorTypeInfo::size()> m_bicubicParams;
	std::array<QPair<int, int>, InterpolatorTypeInfo::size()> m_lanczosParams;
};

class OpenGLFramebufferObject {
public:
	OpenGLFramebufferObject(const QSize &size, int target = GL_TEXTURE_2D)
	: OpenGLFramebufferObject(size, OpenGLCompat::textureFormat(GL_BGRA), target) {}
	OpenGLFramebufferObject(const QSize &size, const OpenGLTextureFormat &format, int target = GL_TEXTURE_2D) {
		auto f = func();
		f->glGenFramebuffers(1, &m_id);
		f->glBindFramebuffer(GL_FRAMEBUFFER, m_id);
		m_texture.width = size.width();
		m_texture.height = size.height();
		m_texture.target = target;
		m_texture.format = format;
		m_texture.generate();
		m_texture.allocate();
		f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texture.id, 0);
		QOpenGLFramebufferObject::bindDefault();
	}
	virtual ~OpenGLFramebufferObject() {
		m_texture.delete_();
		func()->glDeleteFramebuffers(1, &m_id);
	}
	QRect rect() const { return {0, 0, width(), height()}; }
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

class OpenGLTextureShaderProgram : public QOpenGLShaderProgram {
	enum {vPosition, vCoord, vColor};
public:
	OpenGLTextureShaderProgram(QObject *parent = nullptr): QOpenGLShaderProgram(parent) { }
	void setFragmentShader(const QByteArray &code) {
		if (!m_frag)
			m_frag = addShaderFromSourceCode(QOpenGLShader::Fragment, code);
	}
	void setVertexShader(const QByteArray &code) {
		if (!m_vertex) {
			m_vertex = addShaderFromSourceCode(QOpenGLShader::Vertex, code);
			m_hasColor = code.contains("vColor");
		}
	}
	bool link() override {
		bindAttributeLocation("vCoord", vCoord);
		bindAttributeLocation("vPosition", vPosition);
		if (m_hasColor)
			bindAttributeLocation("vColor", vColor);
		return QOpenGLShaderProgram::link();
	}
	void setTextureCount(int textures) {
		if (textures > m_vPositions.size()/(2*4)) {
			textures *= 1.5;
			m_vCoords.resize(2*4*textures);
			m_vPositions.resize(2*4*textures);
			if (m_hasColor)
				m_vColors.resize(4*textures);
		}
	}
	void uploadPosition(int i, const QPointF &p1, const QPointF &p2) {
		uploadRect(m_vPositions.data(), i, p1, p2);
	}
	void uploadPosition(int i, const QRectF &rect) {
		uploadRect(m_vPositions.data(), i, rect.topLeft(), rect.bottomRight());
	}
	void uploadCoord(int i, const QPointF &p1, const QPointF &p2) {
		uploadRect(m_vCoords.data(), i, p1, p2);
	}
	void uploadCoord(int i, const QRectF &rect) {
		uploadRect(m_vCoords.data(), i, rect.topLeft(), rect.bottomRight());
	}
	void uploadColor(int i, quint32 color) {
		auto p = m_vColors.data() + 4*i;
		*p++ = color; *p++ = color; *p++ = color; *p++ = color;
	}
	void begin() {
		bind();
		enableAttributeArray(vPosition);
		enableAttributeArray(vCoord);
		setAttributeArray(vCoord, m_vCoords.data(), 2);
		setAttributeArray(vPosition, m_vPositions.data(), 2);
		if (m_hasColor) {
			enableAttributeArray(vColor);
			setAttributeArray(vColor, GL_UNSIGNED_BYTE, m_vColors.data(), 4);
		}
	}
	void end() {
		disableAttributeArray(vCoord);
		disableAttributeArray(vPosition);
		if (m_hasColor)
			disableAttributeArray(vColor);
		release();
	}
	void reset() { removeAllShaders(); m_frag = m_vertex = false; }
private:
	void uploadRect(float *p, int i, const QPointF &p1, const QPointF &p2) {
		p += 4*2*i;
		*p++ = p1.x(); *p++ = p1.y();
		*p++ = p1.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p1.y();
	}
	QVector<float> m_vPositions, m_vCoords;
	QVector<quint32> m_vColors;
	bool m_hasColor = false, m_frag = false, m_vertex = false;
};

class OpenGLOffscreenContext : public QOpenGLContext {
public:
	OpenGLOffscreenContext() {
		m_surface.setFormat(format());
		m_surface.create();
	}
	bool makeCurrent() { return QOpenGLContext::makeCurrent(&m_surface); }
private:
	QOffscreenSurface m_surface;
};

#endif // OPENGLCOMPAT_HPP
