#ifndef PIXMAPOVERLAY_HPP
#define PIXMAPOVERLAY_HPP

#include "overlay.hpp"

class PixmapOverlay : public Overlay {
	Q_OBJECT
public:
	PixmapOverlay(QGLWidget *video);
	~PixmapOverlay();
	void setArea(const QRect &bg, const QRect &video);
	qint64 addOsd(OsdRenderer *osd);
	void render(QPainter *painter);
	Type type() const {return Pixmap;}
	QList<OsdRenderer*> takeOsds();
private slots:
	void cache();
private:
	struct Data;
	Data *d;
};

#endif // PIXMAPOVERLAY_HPP
