#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include <QtCore/QObject>
#include <QtOpenGL/QGLWidget>
#include "osdrenderer.hpp"

class QGLWidget;

class Overlay : public QObject {
	Q_OBJECT
public:
	enum Type {Auto, FramebufferObject, /*PixelBuffer, */Pixmap};
	Overlay(QGLWidget *video): m_video(video) {}
	~Overlay() {}
	QGLWidget *video() const {return m_video;}
	virtual void setArea(const QRect &bg, const QRect &video) = 0;
	virtual qint64 addOsd(OsdRenderer *osd) = 0;
	virtual void render(QPainter *painter) = 0;
	virtual Type type() const = 0;
	static Type guessType(int hint);
	static Overlay *create(QGLWidget *video, Type type = Auto);
	static QString typeToString(Type type);
	virtual QList<OsdRenderer*> takeOsds() = 0;
protected:
	QSize cachedSize(const QSize &size) {return OsdRenderer::cachedSize(size);}
private:
	QGLWidget *m_video;
};

#endif // OVERLAY_HPP
