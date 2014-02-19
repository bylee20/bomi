#include "openglmisc.hpp"
#include "openglcompat.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

void OpenGLTexture2D::initialize(int width, int height, OGL::TransferFormat transfer, const void *data) {
	initialize(width, height, OpenGLCompat::textureTransferInfo(transfer), data);
}

void OpenGLTextureBase::setFilter(OGL::Filter filter) {
	glTexParameterf(m_target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameterf(m_target, GL_TEXTURE_MIN_FILTER, filter);
}

void OpenGLTextureBase::setWrapMode(OGL::WrapMode wrap) {
	switch (m_target) {
	case GL_TEXTURE_3D:
		glTexParameterf(m_target, GL_TEXTURE_WRAP_R, wrap);
	case OGL::Target2D:
	case OGL::TargetRectangle:
		glTexParameterf(m_target, GL_TEXTURE_WRAP_T, wrap);
	case OGL::Target1D:
		glTexParameterf(m_target, GL_TEXTURE_WRAP_S, wrap);
		break;
	default:
		break;
	}
}

void OpenGLTextureBase::create(OGL::Filter filter, OGL::WrapMode wrap) {
	glGenTextures(1, &m_id);
	OpenGLTextureBaseBinder binder(m_target, OGL::bindingTarget(m_target));
	binder.bind(this);
	setFilter(filter);
	setWrapMode(wrap);
}

QImage OpenGLTexture2D::toImage() const {
	if (isEmpty())
		return QImage();
	OpenGLTextureBinder<OGL::Target2D> binder(const_cast<OpenGLTexture2D*>(this));
	QImage image(size(), QImage::Format_ARGB32);
	image.fill(0x0);
	glGetTexImage(target(), 0, m_info.transfer.format, m_info.transfer.type, image.bits());
	return image;
}


QImage OpenGLFramebufferObject::toImage() const {
	if (!m_texture.isValid())
		return QImage();
	const bool wasBound = isBound();
	if (!wasBound)
		const_cast<OpenGLFramebufferObject*>(this)->bind();
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	QImage image(size(), QImage::Format_ARGB32);
	glReadPixels(0, 0, width(), height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image.bits());
	if (!wasBound)
		const_cast<OpenGLFramebufferObject*>(this)->release();
	return image;
}
