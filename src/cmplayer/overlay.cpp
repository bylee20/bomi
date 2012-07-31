#include "overlay.hpp"
#include "videorenderer.hpp"
#include "osdrenderer.hpp"
#include <QtOpenGL/QGLShaderProgram>
#include <QtCore/QDebug>

extern "C" void *fast_memcpy(void *to, const void *from, size_t len);

static QGLShaderProgram *shader = nullptr;

struct ScreenOsdWrapper : public OsdWrapper {
	bool cached;
	ScreenOsdWrapper(OsdRenderer *renderer) {m_renderer = renderer;}
	void free() {OsdWrapper::free(); cached = false;}
	void cache() {
		if (!m_renderer || !count())
			return;
		m_renderer->prepareToRender(QPointF(0, 0));
		const QSize size = m_renderer->size().toSize();
		const int length = size.width()*size.height()*4;
		if (!(empty = length <= 0)) {
			if (m_buffer.length() < length || m_buffer.length() > length*2)
				m_buffer.resize(length);
			m_buffer.fill(0x0, length);
			m_image = QImage((uchar*)m_buffer.data(), size.width(), size.height(), QImage::Format_ARGB32_Premultiplied);
			if (!m_image.isNull()) {
				QPainter painter(&m_image);
				m_renderer->render(&painter, QPointF(0, 0));
				painter.end();
				bind(m_image.size(), m_image.bits());
			}
		}
		cached = true;
	}
	OsdRenderer *renderer() const {return m_renderer;}
	void render(const QPointF &rpos, const QPointF &fpos) {
		QGLShaderProgram *shader = ::shader;
		if (!shader)
			return;
		shader->bind();
		shader->setUniformValue("tex", 4);
		shader->setUniformValue("dxdy", dx(), dy());
		if (renderer()->style().has_shadow) {
			shader->setUniformValue("shadow_color", renderer()->style().shadow_color);
			QPointF pos;
			pos.rx() = pos.ry() = renderer()->scale()*0.05;
			shader->setUniformValue("shadow_offset", pos);
		} else {
			shader->setUniformValue("shadow_color", 0.f, 0.f, 0.f, 0.f);
			shader->setUniformValue("shadow_offset", 0.f, 0.f);
		}
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, texture());
		float textureCoords[] = {
			0.f, 0.f,					sub_x(), 0.f,
			sub_x(), sub_y(),			0.f, sub_y()
		};
		const QPointF pos = renderer()->posHint() + (renderer()->letterboxHint() ? rpos : fpos);
		const QSizeF size = this->size();
		const float x1 = pos.x(), y1 = pos.y();
		const float x2 = x1 + size.width(), y2 = y1 + size.height();
		float vertexCoords[] = {
			x1, y1,
			x2, y1,
			x2, y2,
			x1, y2
		};

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		shader->release();
	}

private:
	GLint _internalFormat(int) const {return 4;}
	GLenum _format(int) const {return GL_BGRA;}
	GLenum _type(int) const {return GL_UNSIGNED_INT_8_8_8_8_REV;}
	int _count() const {return 1;}
	QImage m_image;
	QByteArray m_buffer;
	OsdRenderer *m_renderer;
};


struct Overlay::Data {
	QGLWidget *ctx;
	QMap<OsdRenderer*, ScreenOsdWrapper*> osds;
	VideoScreen *screen;
	QRect renderable;
	QRect frame;
	bool pending;
};


Overlay::Overlay(VideoScreen *screen)
: d(new Data) {
	d->screen = screen;
	d->pending = false;
}

Overlay::~Overlay() {
	qDeleteAll(d->osds);
	delete d;
}

void Overlay::cache() {
	for (auto osd : d->osds)
		osd->cache();
}

void Overlay::setScreen(VideoScreen *screen) {
	if (d->screen != screen) {
		if (!screen) {
			d->screen->makeCurrent();
			for (auto osd : d->osds)
				osd->free();
			delete shader;
			shader = nullptr;
		}
		if ((d->screen = screen)) {
			d->screen->makeCurrent();
			initializeGLFunctions(d->screen->context());
			shader = new QGLShaderProgram(d->screen->context());
			shader->addShaderFromSourceCode(QGLShader::Fragment,
				"uniform sampler2D tex;"
				"uniform vec2 dxdy;"
				"uniform vec2 shadow_offset;"
				"uniform vec4 shadow_color;"
				"void main() {"
				"	vec4 top = texture2D(tex, gl_TexCoord[0].xy);"
				"	float alpha = texture2D(tex, gl_TexCoord[0].xy - dxdy*shadow_offset).a;"
				"	gl_FragColor = top + alpha*shadow_color*(1.0 - top.a);"
				"}");
			shader->link();
			for (auto osd : d->osds)
				osd->alloc();
		}
	}
}

QGLWidget *Overlay::screen() const {
	return d->screen;
}

void Overlay::setArea(const QRect &renderable, const QRect &frame) {
	if (d->renderable == renderable && d->frame == frame)
		return;
	d->pending = true;
	d->renderable = renderable;
	d->frame = frame;
	d->pending = true;
	for (auto it = d->osds.begin(); it != d->osds.end(); ++it) {
		if (it.key()->setArea(renderable.size(), frame.size()))
			it.value()->cached = false;
	}
	d->pending = false;
	if (d->screen)
		d->screen->redraw();
}

void Overlay::update() {
	if (d->pending)
		return;
	auto it = d->osds.find(qobject_cast<OsdRenderer*>(sender()));
	if (it != d->osds.end())
		it.value()->cached = false;
	if (d->screen)
		d->screen->redraw();
}

void Overlay::add(OsdRenderer *osd) {
	osd->setArea(d->renderable.size(), d->frame.size());
	connect(osd, SIGNAL(needToRerender()), this, SLOT(update()));
	connect(osd, SIGNAL(destroyed()), this, SLOT(onOsdDeleted()));
	auto wrapper = new ScreenOsdWrapper(osd);
	d->osds.insert(osd, wrapper);
	if (d->screen) {
		d->screen->makeCurrent();
		wrapper->alloc();
	}
}

void Overlay::onOsdDeleted() {
	take(static_cast<OsdRenderer*>(sender()));
}

OsdRenderer *Overlay::take(OsdRenderer *osd) {
	delete d->osds.take(osd);
	return osd;
}

void Overlay::render(QPainter *painter) {
	for (auto it = d->osds.begin(); it != d->osds.end(); ++it)
		it.key()->render(painter, it.key()->posHint());
}

void Overlay::renderToScreen() {
	d->screen->makeCurrent();
	const QPointF rpos = d->renderable.topLeft();
	const QPointF fpos = d->frame.topLeft();
	for (auto it = d->osds.begin(); it != d->osds.end(); ++it) {
		auto osd = it.value();
		if (!osd->cached)
			osd->cache();
		if (!osd->empty)
			osd->render(rpos, fpos);
	}
}



//#include <QtGui/QTextDocument>

//struct TextDrawerV2 {
//	TextDrawerV2(const OsdStyle *style, QTextDocument *doc)
//	: m_style(style), m_doc(doc), m_border(-1) {}
//	QSizeF size() const {return m_size;}
//	void update(double border, double width, const RichString &text) {
//		m_text = text;
//		m_doc->setHtml(text.toString());
//		double w = m_border*2.0;
//		double h = m_border*2.0;
//		m_doc->setTextWidth(width - w);
//		if (width > 0.0) {
//			m_size.setWidth(m_doc->idealWidth() + w);
//			m_size.setHeight(m_doc->size().height() + h);
//		} else
//			m_size = QSizeF();
//		postUpdate();
//	}
//	virtual ~TextDrawer() {}
//	virtual void draw(QPainter *painter, const QPointF &pos);
//protected:
//	virtual void postUpdate() {}
//	QVector<QPointF> m_points;
//	void drawTextTo(QPainter *painter, const QPointF &origin, const QColor &color, bool overwrite = true) {
//		painter->save();
//		painter->translate(origin);
//		m_doc->setHtml(coloredHtml(color, overwrite));
//		m_doc->drawContents(painter);
//		painter->restore();
//	}
//	void drawThickTo(QPaintDevice *dev, QPixmap &buffer
//			, const QPointF &origin, const QPointF &offset, const QColor &color) {
//		buffer.fill(Qt::transparent);
//		QPainter painter(&buffer);
//		drawTextTo(&painter, origin, color);
//		painter.end();
//		painter.begin(dev);
//		painter.translate(offset);
//		for (int i=0; i<m_points.size(); ++i)
//			painter.drawPixmap(m_points[i], buffer);
//	}
//	void drawThickTo(QPaintDevice *dev, QImage &buffer
//			, const QPointF &origin, const QPointF &offset, const QColor &color) {
//		buffer.fill(0x0);
//		QPainter painter(&buffer);
//		drawTextTo(&painter, origin, color);
//		painter.end();
//		painter.begin(dev);
//		painter.translate(offset);
//		for (int i=0; i<m_points.size(); ++i)
//			painter.drawImage(m_points[i], buffer);
//	}
//	QString coloredHtml(const QColor &color, bool overwrite) const {
//		QString html = QString("<font color='%1'>").arg(color.name());
//		if (overwrite) {
//			static const QRegExp rxColor("\\s+[cC][oO][lL][oO][rR]\\s*=\\s*[^>\\s\\t]+");
//			html += QString(m_text.toString()).remove(rxColor);
//		} else
//			html += m_text.toString();
//		html += "</font>";
//		return html;
//	}
//	QPointF origin() const {
//		double x = 0.0;
//		const Qt::Alignment alignment = m_doc->defaultTextOption().alignment();
//		if (alignment & Qt::AlignLeft) {
//			x += m_border;
//		} else if (alignment & Qt::AlignRight) {
//			x -= m_border;
//			x -= (m_doc->textWidth() - m_doc->idealWidth());
//		} else
//			x -= (m_doc->textWidth() - m_doc->idealWidth())*0.5;
//		double y = m_border;
//		return QPointF(x, y);
//	}
//	const OsdStyle *m_style;
//	QTextDocument *m_doc;
//	double m_border;
//	QSizeF m_size;
//	RichString m_text;
//};
