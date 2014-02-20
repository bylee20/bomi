#ifndef OPENGLMISC_HPP
#define OPENGLMISC_HPP

#include "stdafx.hpp"

#ifdef PixelFormat
#undef PixelFormat
#endif

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define GL_UNSIGNED_SHORT_8_8_MESA      0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA  0x85BB
#endif

class OpenGLTextureBase;
class OpenGLTexture1D;
class OpenGLTexture2D;
class OpenGLTexture3D;

namespace OGL {
enum Target {
	Target1D        = GL_TEXTURE_1D,
	Target2D        = GL_TEXTURE_2D,
	Target3D        = GL_TEXTURE_3D,
	TargetRectangle = GL_TEXTURE_RECTANGLE
};
enum Binding {
	Binding1D = GL_TEXTURE_BINDING_1D,
	Binding2D = GL_TEXTURE_BINDING_2D,
	Binding3D = GL_TEXTURE_BINDING_3D,
	BindingRectangle = GL_TEXTURE_BINDING_RECTANGLE,
};

static inline Binding bindingTarget(Target target) {
	switch (target) {
	case Target1D: return Binding1D;
	case Target2D: return Binding2D;
	case Target3D: return Binding3D;
	case TargetRectangle: return BindingRectangle;
	default: return (Binding)GL_NONE;
	}
}

template<Target target> struct target_trait { using texture_type = OpenGLTextureBase; };
template<> struct target_trait<Target1D> {
	static constexpr Target target = Target1D;
	static constexpr Binding binding = Binding1D;
	using texture_type = OpenGLTexture1D;
};
template<> struct target_trait<Target2D> {
	static constexpr Target target = Target2D;
	static constexpr Binding binding = Binding2D;
	using texture_type = OpenGLTexture2D;
};
template<> struct target_trait<Target3D> {
	static constexpr Target target = Target3D;
	static constexpr Binding binding = Binding3D;
	using texture_type = OpenGLTexture3D;
};
template<> struct target_trait<TargetRectangle> {
	static constexpr Target target = TargetRectangle;
	static constexpr Binding binding = BindingRectangle;
	using texture_type = OpenGLTexture2D;
};

enum TransferType { // pixel format
	NoTransferType       = GL_NONE,
	UInt8                = GL_UNSIGNED_BYTE,
	UInt16               = GL_UNSIGNED_SHORT,
	Float32              = GL_FLOAT,
	UInt32_8_8_8_8       = GL_UNSIGNED_INT_8_8_8_8,
	UInt32_8_8_8_8_Rev   = GL_UNSIGNED_INT_8_8_8_8_REV,
	UInt16_8_8_Apple     = GL_UNSIGNED_SHORT_8_8_APPLE,
	UInt16_8_8_Rev_Apple = GL_UNSIGNED_SHORT_8_8_REV_APPLE,
	UInt16_8_8_Mesa      = GL_UNSIGNED_SHORT_8_8_MESA,
	UInt16_8_8_Rev_Mesa  = GL_UNSIGNED_SHORT_8_8_REV_MESA
};

enum TextureFormat { // internal format
	NoTextureFormat        = GL_NONE,
	R8_UNorm               = GL_R8,
	RG8_UNorm              = GL_RG8,
	RGB8_UNorm             = GL_RGB8,
	RGBA8_UNorm            = GL_RGBA8,
	R16_UNorm              = GL_R16,
	RG16_UNorm             = GL_RG16,
	RGB16_UNorm            = GL_RGB16,
	RGBA16_UNorm           = GL_RGBA16,
	Luminance8_UNorm       = GL_LUMINANCE8,
	Luminance16_UNorm      = GL_LUMINANCE16,
	LuminanceAlpha8_UNorm  = GL_LUMINANCE8_ALPHA8,
	LuminanceAlpha16_UNorm = GL_LUMINANCE16_ALPHA16,
	YCbCr_UNorm_Mesa       = GL_YCBCR_MESA,
	R16F                   = GL_R16F,
	RG16F                  = GL_RG16F,
	RGB16F                 = GL_RGB16F,
	RGBA16F                = GL_RGBA16F,
	R32F                   = GL_R32F,
	RG32F                  = GL_RG32F,
	RGB32F                 = GL_RGB32F,
	RGBA32F                = GL_RGBA32F,
};
enum TransferFormat {
	NoTransferFormat = GL_NONE,
	Red              = GL_RED,
	RG               = GL_RG,
	RGB              = GL_RGB,
	BGR              = GL_BGR,
	RGBA             = GL_RGBA,
	BGRA             = GL_BGRA,
	Luminance        = GL_LUMINANCE,
	LuminanceAlpha   = GL_LUMINANCE_ALPHA,
	YCbCr_422_Apple  = GL_YCBCR_422_APPLE,
	YCbCr_Mesa       = GL_YCBCR_MESA,
	OneComponent,
	TwoComponents
};
enum Filter {
	Nearest = GL_NEAREST,
	Linear  = GL_LINEAR,
};

enum WrapMode {
	Repeat         = GL_REPEAT,
	MirroredRepeat = GL_MIRRORED_REPEAT,
	ClampToEdge    = GL_CLAMP_TO_EDGE,
	ClampToBorder  = GL_CLAMP_TO_BORDER
};

enum WrapDirection {
	DirectionS = GL_TEXTURE_WRAP_S,
	DirectionT = GL_TEXTURE_WRAP_T,
	DirectionR = GL_TEXTURE_WRAP_R
};

struct TransferInfo {
	TransferInfo() {}
	TransferInfo(TransferFormat format, TransferType type): format(format), type(type) {}
	TransferFormat format = OGL::BGRA;
	TransferType type = OGL::UInt32_8_8_8_8_Rev;
};
}


struct OpenGLTextureTransferInfo {
	OpenGLTextureTransferInfo() {}
	OpenGLTextureTransferInfo(OGL::TextureFormat texture, OGL::TransferFormat transferFormat, OGL::TransferType transferType)
	: texture(texture), transfer(transferFormat, transferType) {}
	virtual ~OpenGLTextureTransferInfo() {}
	OGL::TextureFormat texture = OGL::RGBA8_UNorm;
	OGL::TransferInfo transfer;
};

class OpenGLTextureBase {
public:
	virtual ~OpenGLTextureBase() {}
	GLuint id() const { return m_id; }
	OGL::Target target() const { return m_target; }
	void create(OGL::Filter filter, OGL::WrapMode wrap = OGL::ClampToEdge);
	void create(OGL::WrapMode wrap, OGL::Filter filter = OGL::Linear) { create(filter, wrap); }
	void create() { create(OGL::Linear, OGL::ClampToEdge); }
	void destroy() { glDeleteTextures(1, &m_id); }
	void bind() const { glBindTexture(m_target, m_id); }
	void setFilter(OGL::Filter filter);
	void setWrapMode(OGL::WrapMode wrap);
	bool isValid() const { return m_id != GL_NONE; }
	OGL::TextureFormat format() const { return m_info.texture; }
	const OGL::TransferInfo &transfer() const { return m_info.transfer; }
protected:
	OpenGLTextureBase(OGL::Target target): m_target(target) {}
	OpenGLTextureTransferInfo m_info;
private:
	GLuint m_id = GL_NONE;
	friend class OpenGLFramebufferObject;
	OGL::Target m_target = OGL::Target2D;
};

template<OGL::Target _target>
class OpenGLTextureBinder {
	using trait = OGL::target_trait<_target>;
	using Texture = typename trait::texture_type;
public:
	inline OpenGLTextureBinder() { glGetIntegerv(binding(), &m_restore); }
	inline OpenGLTextureBinder(Texture *texture): OpenGLTextureBinder() { bind(texture); }
	inline ~OpenGLTextureBinder() { glBindTexture(_target, m_restore); }
	static inline constexpr OGL::Target target() { return _target; }
	static inline constexpr OGL::Binding binding() { return trait::binding; }
	inline void bind(Texture *texture) {
		Q_ASSERT(texture->target() == _target);
		m_texture = texture; m_texture->bind();
	}
	inline Texture &operator*() const { return *m_texture; }
	inline Texture *operator->() const { return m_texture; }
private:
	GLint m_restore = GL_NONE;
	Texture *m_texture = nullptr;
};

class OpenGLTextureBaseBinder {
public:
	inline OpenGLTextureBaseBinder(OGL::Target target, OGL::Binding binding)
		: m_target(target), m_binding(binding) { glGetIntegerv(binding, &m_restore); }
	inline ~OpenGLTextureBaseBinder() { glBindTexture(m_target, m_restore); }
	inline OGL::Target target() { return m_target; }
	inline OGL::Binding binding() { return m_binding; }
	inline void bind(OpenGLTextureBase *texture) {
		Q_ASSERT(texture->target() == m_target);
		m_texture = texture; m_texture->bind();
	}
	inline OpenGLTextureBase &operator*() const { return *m_texture; }
	inline OpenGLTextureBase *operator->() const { return m_texture; }
private:
	GLint m_restore = GL_NONE;
	OGL::Target m_target;
	OGL::Binding m_binding;
	OpenGLTextureBase *m_texture = nullptr;
};

class OpenGLTexture1D : public OpenGLTextureBase {
public:
	OpenGLTexture1D(): OpenGLTextureBase(OGL::Target1D) {}
	int width() const { return m_width; }
	void initialize(int width, const OpenGLTextureTransferInfo &info, const void *data = nullptr) {
		m_info = info; m_width = width;
		if (!isEmpty())
			glTexImage1D(target(), 0, m_info.texture, m_width, 0, m_info.transfer.format, m_info.transfer.type, data);
	}
	bool isEmpty() const { return !isValid() || m_width <= 0; }
private:
	int m_width = 0;
};

class OpenGLTexture2D : public OpenGLTextureBase {
	friend class OpenGLFramebufferObject;
public:
	OpenGLTexture2D(OGL::Target target = OGL::Target2D): OpenGLTextureBase(target) {}
	bool isRectangle() const { return target() == OGL::TargetRectangle; }
	int width() const { return m_width; }
	int height() const { return m_height; }
	QSize size() const { return {m_width, m_height}; }
	void initialize(const QSize &size, const OpenGLTextureTransferInfo &info, const void *data = nullptr) { initialize(size.width(), size.height(), info, data); }
	void initialize(int width, int height, OGL::TransferFormat transfer, const void *data = nullptr);
	void initialize(int width, int height, const OpenGLTextureTransferInfo &info, const void *data = nullptr) {
		m_info = info; initialize(width, height, data);
	}
	void initialize(const QSize &size, const void *data = nullptr) { initialize(size.width(), size.height(), data); }
	void initialize(int width, int height, const void *data = nullptr) {
		m_width = width; m_height = height; initialize(data);
	}
	void setAttributes(int width, int height, const OpenGLTextureTransferInfo &info) {
		m_width = width; m_height = height; m_info = info;
	}
	void initialize(const void *data = nullptr) {
		if (!isEmpty())
			glTexImage2D(target(), 0, m_info.texture, m_width, m_height, 0, m_info.transfer.format, m_info.transfer.type, data);
	}
	bool isEmpty() const { return !isValid() || m_width <= 0 || m_height <= 0; }
	void upload(int x, int y, int width, int height, const void *data) {
		glTexSubImage2D(target(), 0, x, y, width, height, m_info.transfer.format, m_info.transfer.type, data);
	}
	void upload(const QRect &rect, const void *data) { upload(rect.x(), rect.y(), rect.width(), rect.height(), data); }
	void upload(int width, int height, const void *data) { upload(0, 0, width, height, data); }
	void upload(const void *data) { upload(0, 0, m_width, m_height, data); }
	QImage toImage() const;
	int &plane() { return m_plane; }
	int plane() const { return m_plane; }
	const QPointF &correction() const { return m_correction; }
	QPointF &correction() { return m_correction; }
private:
	friend class VideoFrameShader;
	int m_width = 0, m_height = 0, m_plane = 0;
	QPointF m_correction = {1.0, 1.0};
};

class OpenGLFramebufferObject : public QOpenGLFramebufferObject {
public:
	OpenGLFramebufferObject(const QSize &size, OGL::TextureFormat internal = OGL::RGBA8_UNorm)
	: QOpenGLFramebufferObject(size, NoAttachment, OGL::Target2D, internal) {
		m_texture.m_id = QOpenGLFramebufferObject::texture();
		m_texture.m_width = size.width();
		m_texture.m_height = size.height();
		m_texture.m_info.texture = internal;
		m_texture.m_info.transfer.format = OGL::RGBA;
		m_texture.m_info.transfer.type = OGL::UInt8;
		if (isValid())
			OpenGLTextureBinder<OGL::Target2D>(&m_texture)->setFilter(OGL::Linear);
	}
	const OpenGLTexture2D &texture() const { return m_texture; }
	void getCoords(double &x1, double &y1, double &x2, double &y2) {
		if (m_texture.target() == OGL::TargetRectangle) {
			x1 = y1 = 0; x2 = m_texture.width(); y2 = m_texture.height();
		} else { x1 = y1 = 0; x2 = y2 = 1; }
	}
	QImage toImage() const;
private:
	OpenGLTexture2D m_texture;
};

#define ENUM_CASE(e) case e: return QByteArray::fromRawData(#e, sizeof(e)-1)
static inline QByteArray _ToLog(OGL::TransferType type) {
	switch (type) {
	ENUM_CASE(OGL::UInt8);
	ENUM_CASE(OGL::UInt16);
	ENUM_CASE(OGL::Float32);
	ENUM_CASE(OGL::UInt32_8_8_8_8);
	ENUM_CASE(OGL::UInt32_8_8_8_8_Rev);
#ifdef Q_OS_MAC
	ENUM_CASE(OGL::UInt16_8_8_Apple);
	ENUM_CASE(OGL::UInt16_8_8_Rev_Apple);
#else
	ENUM_CASE(OGL::UInt16_8_8_Mesa);
	ENUM_CASE(OGL::UInt16_8_8_Rev_Mesa);
	ENUM_CASE(OGL::NoTransferType);
#endif
	}
	return QByteArray("OGL::InvalidTransferType");
}

static inline QByteArray _ToLog(OGL::TextureFormat format) {
	switch (format) {
	ENUM_CASE(OGL::R8_UNorm);
	ENUM_CASE(OGL::RG8_UNorm);
	ENUM_CASE(OGL::RGB8_UNorm);
	ENUM_CASE(OGL::RGBA8_UNorm);
	ENUM_CASE(OGL::R16_UNorm);
	ENUM_CASE(OGL::RG16_UNorm);
	ENUM_CASE(OGL::RGB16_UNorm);
	ENUM_CASE(OGL::RGBA16_UNorm);
	ENUM_CASE(OGL::Luminance8_UNorm);
	ENUM_CASE(OGL::Luminance16_UNorm);
	ENUM_CASE(OGL::LuminanceAlpha8_UNorm);
	ENUM_CASE(OGL::LuminanceAlpha16_UNorm);
	ENUM_CASE(OGL::YCbCr_UNorm_Mesa);
	ENUM_CASE(OGL::R16F);
	ENUM_CASE(OGL::RG16F);
	ENUM_CASE(OGL::RGB16F);
	ENUM_CASE(OGL::RGBA16F);
	ENUM_CASE(OGL::R32F);
	ENUM_CASE(OGL::RG32F);
	ENUM_CASE(OGL::RGB32F);
	ENUM_CASE(OGL::RGBA32F);
	ENUM_CASE(OGL::NoTextureFormat);
	}
	return QByteArray("OGL::InvalidTextureFormat");
}

static inline QByteArray _ToLog(OGL::TransferFormat format) {
	switch (format) {
	ENUM_CASE(OGL::Red);
	ENUM_CASE(OGL::RG);
	ENUM_CASE(OGL::RGB);
	ENUM_CASE(OGL::BGR);
	ENUM_CASE(OGL::RGBA);
	ENUM_CASE(OGL::BGRA);
	ENUM_CASE(OGL::Luminance);
	ENUM_CASE(OGL::LuminanceAlpha);
	ENUM_CASE(OGL::YCbCr_422_Apple);
	ENUM_CASE(OGL::YCbCr_Mesa);
	ENUM_CASE(OGL::OneComponent);
	ENUM_CASE(OGL::TwoComponents);
	ENUM_CASE(OGL::NoTransferFormat);
	}
	return QByteArray("OGL::InvalidTransferFormat");
}
#undef ENUM_CASE

#endif // OPENGLMISC_HPP
