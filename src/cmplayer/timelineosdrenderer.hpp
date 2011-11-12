#ifndef TIMELINEOSDRENDERER_HPP
#define TIMELINEOSDRENDERER_HPP

#include "osdrenderer.hpp"

class TimeLineOsdRenderer : public OsdRenderer {
	Q_OBJECT
public:
	TimeLineOsdRenderer();
	~TimeLineOsdRenderer();
	void show(int pos, int duration, int last = 2500);
	void render(QPainter *painter, const QPointF &pos);
	QPointF posHint() const;
	QSizeF size() const;
	void setStyle(const OsdStyle &style);
	const OsdStyle &style() const;
public slots:
	void clear();
private:
	void updateBackgroundSize(const QSizeF &size);
	struct Data;
	Data *d;

};

#endif // TIMELINEOSDRENDERER_HPP
