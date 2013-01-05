#include "logodrawer.hpp"

LogoDrawer::LogoDrawer() {
	m_logo.load(":/img/cmplayer512.png");
	m_logo = m_logo.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	QLinearGradient grad(0.5, 1.0, 0.75, 0.13);
	grad.setColorAt(0.0, qRgb(51, 131, 230));
	grad.setColorAt(1.0, qRgb(110, 202, 247));
	m_bgBrush = QBrush(grad);

	grad = QLinearGradient(0.3, 0.1, 0.5, 0.9);
	grad.setColorAt(0.0, Qt::white);
	grad.setColorAt(1.0, Qt::transparent);
	m_lightBrush = QBrush(grad);

	const double oh = 0.6;
	m_lightPath.moveTo(0.0, 0.0);
	m_lightPath.lineTo(0.0, oh*0.8);
	m_lightPath.cubicTo(0.1, oh*0.9, 0.3, oh, 0.4, oh);
	m_lightPath.cubicTo(0.6, oh, 0.8, oh*0.9, 1.0, oh*0.6);
	m_lightPath.lineTo(1.0, 0.0);
	m_lightPath.closeSubpath();

	m_gl = nullptr;
}

LogoDrawer::~LogoDrawer() {
	bind(nullptr);
}

void LogoDrawer::bind(QGLWidget *gl) {
	if (m_gl == gl)
		return;
	if (m_gl) {
		glDeleteTextures(1, &m_texture);
		m_gl = nullptr;
	}
	if (gl) {
		m_gl = gl;
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, m_logo.width(), m_logo.height()
			, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, m_logo.bits());
	}
}

void LogoDrawer::drawLogo(QPainter */*painter*/, const QRectF &bg) {
	const float w = bg.width(), h = bg.height();
	const float len = qMin(qMin(w, h)*0.7f, (float)m_logo.width());
	const float x = (w-len)*0.5, y = (h-len)*0.5;
	if (m_gl)
		m_gl->drawTexture(QRectF(x, y+len, len, -len), m_texture);
}
#include <QtSvg/QSvgRenderer>

void LogoDrawer::draw(QPainter *painter, const QRectF &bg) {
//	QSvgRenderer renderer(QString("/Users/xylosper/dev/cmplayer/icons/cmplayer.svg"));
//	qDebug() << renderer.isValid();
//	painter->setRenderHint(QPainter::Antialiasing);
//	QImage image(bg.size().toSize(), QImage::Format_ARGB32_Premultiplied);
//	QPainter p(&image);
//	renderer.render(&p, image.rect());
//	painter->fillRect(bg, Qt::white);
////	renderer.render(painter, bg);
//	painter->drawImage(bg, image);
//	return;
	const double w = bg.width();
	const double h = bg.height();

	painter->save();
	painter->setPen(Qt::NoPen);
	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	painter->save();
	painter->scale(w, h);
	painter->fillRect(bg, m_bgBrush);
	painter->restore();

	painter->save();
	painter->scale(w, h);
	painter->setOpacity(0.2);
	painter->setBrush(m_lightBrush);
	painter->drawPath(m_lightPath);
	painter->restore();

	drawLogo(painter, bg);

	painter->restore();
}
