#ifndef LOGOVIEW_HPP
#define LOGOVIEW_HPP

#include "stdafx.hpp"

class QGLWidget;			class QGLFramebufferObject;

class LogoDrawer {
public:
	LogoDrawer();
	~LogoDrawer();
	void draw(QPainter *painter, const QRectF &rect);
private:
	QBrush m_bgBrush, m_lightBrush;
	QPainterPath m_lightPath;
};

#endif // LOGOVIEW_HPP
