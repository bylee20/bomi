#include "subtitledrawer.hpp"

QPointF SubtitleDrawer::pos(const QSizeF &image, const QRectF &area) const {
	QPointF pos(0.0, 0.0);
	if (m_alignment & Qt::AlignBottom) {
		pos.ry() = qMax(0.0, area.height()*(1.0 - m_margin.bottom) - image.height());
	} else if (m_alignment & Qt::AlignVCenter)
		pos.ry() = (area.height() - image.height())*0.5;
	else {
		pos.ry() = area.height()*m_margin.top;
		if (pos.y() + image.height() > area.height())
			pos.ry() = area.height() - image.height();
	}
	if (m_alignment & Qt::AlignHCenter)
		pos.rx() = (area.width() - image.width())*0.5;
	else if (m_alignment & Qt::AlignRight)
		pos.rx() = area.width()*(1.0 - m_margin.right) - image.width();
	else
		pos.rx() = area.width()*m_margin.left;
	pos += area.topLeft();
	return pos;
}

void SubtitleDrawer::setStyle(const SubtitleStyle &style) {
	m_style = style;
	updateStyle(front, style);
	updateStyle(back, style);
	if (style.outline.enabled)
		back.setTextOutline(style.outline.color, style.font.height()*style.outline.width*2.0);
	else
		back.setTextOutline(Qt::NoPen);
}

bool SubtitleDrawer::draw(QImage &image, QSize &imageSize, QPointF &shadowOffset, const QRectF &area, double dpr) {
	if (!hasWords())
		return false;
	const double scale = this->scale(area);
	updateLayoutInfo();
	doLayout(area.width()/scale);
	shadowOffset = m_style.shadow.offset*m_style.font.height()*scale;
	imageSize = QSize(area.width(), front.naturalSize().height()*scale + qAbs(shadowOffset.y()));
	image = QImage(imageSize*dpr, QImage::Format_ARGB32_Premultiplied);
	if (!image.isNull()) {
		image.setDevicePixelRatio(dpr);
		image.fill(0x0);
		QPainter painter(&image);
		if (shadowOffset.y() < 0)
			painter.translate(0, -shadowOffset.y());
		painter.scale(scale*dpr, scale*dpr);
		back.draw(&painter, QPointF(0, 0));
		front.draw(&painter, QPointF(0, 0));
		painter.end();
	} else
		shadowOffset = QPointF(0, 0);
	return !image.isNull();
}
