#include "timelineosdrenderer.hpp"
#include "global.hpp"
#include <QtCore/QTimer>
#include <QtCore/QPoint>
#include <QtGui/QPainter>
#include <QtCore/QDebug>
#include "osdstyle.hpp"

struct TimeLineOsdRenderer::Data {
	int time, duration;
	QTimer clearer;
	QSizeF size;
	QSizeF renderable;
};

TimeLineOsdRenderer::TimeLineOsdRenderer()
: d(new Data) {
	d->time = d->duration = -1;
	d->clearer.setSingleShot(true);
	connect(&d->clearer, SIGNAL(timeout()), this, SLOT(clear()));
	OsdStyle style = this->style();
	style.scale = OsdStyle::Scale::Height;
	style.color.setAlphaF(0.5);
	style.outline_color.setAlphaF(0.5);
	style.size = 0.05;
	setStyle(style);

}

TimeLineOsdRenderer::~TimeLineOsdRenderer() {
	delete d;
}

bool TimeLineOsdRenderer::updateRenderableSize(const QSizeF &renderable) {
	if (d->renderable == renderable)
		return false;
	d->renderable = renderable;
	d->size.setWidth(d->renderable.width()*0.8);
	d->size.setHeight(d->renderable.height()*0.05);
	return true;
}

void TimeLineOsdRenderer::render(QPainter *painter, const QPointF &pos, int /*layers*/) {
	if (d->time < 0 || d->duration <= 0 || d->size.isEmpty() || d->size.isNull())
		return;
	const auto &style = this->style();
	const double b = style.outline_width*d->renderable.height();
	const double rate = (double)d->time/d->duration;
	const double inner_w = (d->size.width()-2.0*b)*rate;
	const QRectF left(0.0, b, b, d->size.height() - 2.0*b - 1.0);
	const QRectF inner(left.topRight(), left.bottomRight() + QPointF(inner_w, 0.0));
	const QRectF right(inner.topRight(), QSizeF(d->size.width() - inner.right(), inner.height()));
	const QRectF top(0.0, 0.0, d->size.width(), b);
	const QRectF bottom(0.0, d->size.height() - b - 1.0, d->size.width(), b);

	painter->save();
	painter->translate(pos);
	painter->fillRect(top, style.outline_color);
	painter->fillRect(bottom, style.outline_color);
	painter->fillRect(left, style.outline_color);
	painter->fillRect(right, style.outline_color);
	painter->fillRect(inner, style.color);
	painter->restore();
}

QPointF TimeLineOsdRenderer::posHint() const {
	const double x = ((double)d->renderable.width() - d->size.width())*0.5;
	const double y = ((double)d->renderable.height() - d->size.height())*0.5;
	return QPointF(x, y);
}

QSizeF TimeLineOsdRenderer::size() const {
	return d->size;
}

void TimeLineOsdRenderer::show(int time, int duration, int last) {
	if (time < 0 || duration <= 0)
		return;
	d->clearer.stop();
	d->time = time;
	d->duration = duration;
	emit needToRerender();
	d->clearer.start(last);
}

void TimeLineOsdRenderer::clear() {
	d->time = d->duration = -1;
	emit needToRerender();
}
