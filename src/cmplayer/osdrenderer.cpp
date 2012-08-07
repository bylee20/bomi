#include "osdrenderer.hpp"
#include "osdstyle.hpp"
#include <math.h>
#include <QtCore/QDebug>
#include "global.hpp"

OsdRenderer::OsdRenderer() {
	m_letterbox = true;
}

OsdRenderer::~OsdRenderer() {
}

double OsdRenderer::scale() const {
	const auto scale = style().scale;
	const auto renderable = renderableSize();
	if (scale == OsdStyle::Scale::Diagonal)
		return diagonal(renderable);
	if (scale == OsdStyle::Scale::Width)
		return renderable.width();
	return renderable.height();
}

void OsdRenderer::setStyle(const OsdStyle &style) {
	m_style = style;
	updateStyle(style);
	emit styleChanged(style);
}

void OsdRenderer::setLetterboxHint(bool letterbox) {
	if (m_letterbox != letterbox) {
		m_letterbox = letterbox;
		if (updateRenderableSize(renderableSize()))
			emit sizeChanged(size());
	}
}

bool OsdRenderer::setArea(const QSize &screen, const QSizeF &frame) {
	m_screen = screen;
	m_frame = frame;
	bool updated = false;
	if ((updated = updateRenderableSize(renderableSize())))
		emit sizeChanged(size());
	return updated;
}
