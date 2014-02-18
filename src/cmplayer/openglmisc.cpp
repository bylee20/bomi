#include "openglmisc.hpp"

QImage OpenGLTexture::toImage() const {
	if (isNull() || isEmpty())
		return QImage();
	bind();
	QImage image(size(), QImage::Format_ARGB32);
	image.fill(0x0);
	glGetTexImage(target, 0, format.pixel, format.type, image.bits());
	unbind();
	return image;
}


QImage OpenGLFramebufferObject::toImage() const {
	if (m_texture.isNull())
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
