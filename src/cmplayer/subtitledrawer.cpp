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
	updateStyle(m_front, style);
	updateStyle(m_back, style);
	if (style.outline.enabled)
		m_back.setTextOutline(style.outline.color, style.font.height()*style.outline.width*2.0);
	else
		m_back.setTextOutline(Qt::NoPen);
}

bool SubtitleDrawer::draw(QImage &image, QPointF &shadowOffset, const RichTextDocument &text, const QRectF &area, double dpr) {
	if (!(m_drawn = text.hasWords()))
		return false;
	const double scale = this->scale(area);
	auto make = [this, text, scale, area] (RichTextDocument &doc, const RichTextDocument &from) {
		doc = from; doc += text; doc.updateLayoutInfo(); doc.doLayout(area.width()/scale);
	};
	RichTextDocument front, back; make(front, m_front); make(back, m_back);
	shadowOffset = m_style.shadow.offset*m_style.font.height()*scale;
	auto nsize = front.naturalSize()*scale;
	const QSize size(nsize.width() + qAbs(shadowOffset.x()), nsize.height() + qAbs(shadowOffset.y()));
	image = QImage(size*dpr, QImage::Format_ARGB32_Premultiplied);
	if (!image.isNull()) {
		image.setDevicePixelRatio(dpr);
		image.fill(0x0);
		QPainter painter(&image);
		QPointF trans{-(area.width() - nsize.width())*0.5, 0.0};
		if (shadowOffset.y() < 0)
			trans.ry() -= shadowOffset.y();
		if (shadowOffset.x() < 0)
			trans.rx() -= shadowOffset.x();
		painter.translate(trans);
		painter.scale(scale, scale);
		back.draw(&painter, QPointF(0, 0));
		front.draw(&painter, QPointF(0, 0));
	} else
		shadowOffset = QPointF(0, 0);
	return m_drawn = !image.isNull();
}
