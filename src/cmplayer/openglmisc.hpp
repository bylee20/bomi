#ifndef OPENGLMISC_HPP
#define OPENGLMISC_HPP

#include "openglcompat.hpp"

#ifndef GL_YCBCR_MESA
#define GL_YCBCR_MESA                   0x8757
#define GL_UNSIGNED_SHORT_8_8_MESA      0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA  0x85BB
#endif

static constexpr QOpenGLTexture::PixelType UInt32_RGBA8 = (QOpenGLTexture::PixelType)GL_UNSIGNED_INT_8_8_8_8;
static constexpr QOpenGLTexture::PixelType UInt32_RGBA8_Rev = (QOpenGLTexture::PixelType)GL_UNSIGNED_INT_8_8_8_8_REV;

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
	void copyAttributesFrom(const OpenGLTexture &other) {
		target = other.target;
		width = other.width;
		height = other.height;
		depth = other.depth;
		format = other.format;
	}
	void setSize(const QSize &size) { width = size.width(); height = size.height(); }
	void generate() { glGenTextures(1, &id); }
	void delete_() { glDeleteTextures(1, &id); }
	void bind() const { glBindTexture(target, id); }
	bool allocate(const void *data = nullptr) const {
		return allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, data);
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
	bool isNull() const { return id != GL_NONE; }
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

#endif // OPENGLMISC_HPP
