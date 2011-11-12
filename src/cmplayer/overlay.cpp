#include "overlay.hpp"
#include "framebufferobjectoverlay.hpp"
#include "pixelbufferoverlay.hpp"
#include "pixmapoverlay.hpp"
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLPixelBuffer>

Overlay::Type Overlay::guessType(int hint) {
	if (hint == FramebufferObject)
		return FramebufferObject;
	if (hint == Pixmap)
		return Pixmap;
#ifdef Q_WS_MAC
	if (QGLFramebufferObject::hasOpenGLFramebufferObjects())
		return FramebufferObject;
#endif
	return Pixmap;
}

Overlay *Overlay::create(QGLWidget *video, Type type) {
	if (type == FramebufferObject)
		return new FramebufferObjectOverlay(video);
	return new PixmapOverlay(video);
}

QString Overlay::typeToString(Type type) {
	switch (type) {
	case FramebufferObject:
		return QString("FramebufferObject");
//	case PixelBuffer:
//		return QString("PixelBuffer");
	case Pixmap:
		return QString("Pixmap");
	default:
		return QString("InvalidType");
	}
}
