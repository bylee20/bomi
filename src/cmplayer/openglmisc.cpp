#include "openglmisc.hpp"

void OpenGLTexture::allocate(OGL::Target target, OGL::TextureFormat format, const QSize &size) {
	if (isEmpty())
		return;
	m_target = target;
	m_format = format;
	m_size = size;
	bind();
	switch (target) {
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		glTexImage2D(target, 0, format, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		break;
	case GL_TEXTURE_3D:
		glTexImage3D(target, 0, format, width(), height(), m_depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		break;
	case GL_TEXTURE_1D:
		glTexImage1D(target, 0, format, width(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		break;
	default:
		break;
	}
	unbind();
}
