#ifndef LOGOVIEW_HPP
#define LOGOVIEW_HPP

#include "stdafx.hpp"

class QGLWidget;			class QGLFramebufferObject;

class LogoDrawer {
public:
	LogoDrawer();
	~LogoDrawer();
	void bind(QGLWidget *gl);
	void draw(QPainter *painter, const QRectF &rect);
private:
	void drawLogo(QPainter *painter, const QRectF &rect);
	QImage m_logo;
	GLuint m_texture;
	QBrush m_bgBrush, m_lightBrush;
	QPainterPath m_lightPath;
	QGLWidget *m_gl;
};

#endif // LOGOVIEW_HPP
