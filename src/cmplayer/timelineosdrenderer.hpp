#ifndef TIMELINEOSDRENDERER_HPP
#define TIMELINEOSDRENDERER_HPP

#include "osdrenderer.hpp"

class TimeLineOsdRenderer : public OsdRenderer {
	Q_OBJECT
public:
	TimeLineOsdRenderer();
	~TimeLineOsdRenderer();
	void show(int pos, int duration, int last = 2500);
	void render(QPainter *painter, const QPointF &pos, int layers);
	QPointF posHint() const;
	QSizeF size() const;
	double scale() const {return size().height();}
public slots:
	void clear();
private:
	bool updateRenderableSize(const QSizeF &renderable);
	struct Data;
	Data *d;
};



#endif // TIMELINEOSDRENDERER_HPP
