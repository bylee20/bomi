#include "osdrenderer.hpp"
#include "osdstyle.hpp"
#include <math.h>
#include <QtCore/QDebug>

int OsdRenderer::cachedSize(double size) {
	return qMax(128, 1 << qRound(log2(size) + 0.5));
}

void OsdRenderer::setLetterboxHint(bool letterbox) {
	if (m_letterbox != letterbox) {
		m_letterbox = letterbox;
		updateBackgroundSize(backgroundSize());
	}
}

void OsdRenderer::setBackgroundSize(const QSize &screen, const QSizeF &video) {
	m_screen = screen;
	m_video = video;
	updateBackgroundSize(backgroundSize());
}
