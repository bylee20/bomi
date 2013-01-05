#include "overlay.hpp"
#include "videorenderer.hpp"
#include "osdrenderer.hpp"

extern "C" void *fast_memcpy(void *to, const void *from, size_t len);

struct ScreenOsdWrapper : public OsdWrapper {
	bool m_needToUpload = false;
	ScreenOsdWrapper(OsdRenderer *renderer) {m_renderer = renderer; m_renderer->setWrapper(this);}
	void cache() {
		if (!m_renderer || !count())
			return;
		QPointF pos(0, 0);
		const QPointF shadow = renderer()->style().shadow_offset*renderer()->scale();
		if (shadow.x() < 0)
			pos.rx() = -shadow.x();
		if (shadow.y() < 0)
			pos.ry() = -shadow.y();
		m_renderer->prepareToRender(pos);
		m_size[m_drawing] = m_renderer->size().toSize();
		m_size[m_drawing].rwidth() += qAbs(shadow.x());
		m_size[m_drawing].rheight() += qAbs(shadow.y());
		QByteArray &buffer = m_buffer[m_drawing];
		const int length = m_size[m_drawing].width()*m_size[m_drawing].height()*4;
		m_empty = length <= 0;
		if (!m_empty && (buffer.length() < length || buffer.length() > length*2))
			buffer.resize(length);
		if (!m_empty) {
			buffer.fill(0x0, length);
			QImage image((uchar*)buffer.data(), m_size[m_drawing].width(), m_size[m_drawing].height(), QImage::Format_ARGB32_Premultiplied);
			Q_ASSERT(!image.isNull());
			QPainter painter(&image);
			m_renderer->render(&painter, pos);
			painter.end();
			qSwap(m_drawing, m_interm);
			if (!m_needToUpload)
				m_needToUpload = true;
		}
	}
	OsdRenderer *renderer() const {return m_renderer;}
	void render(QGLFunctions *func, const QPointF &rpos, const QPointF &fpos, QGLShaderProgram *shader) {
		if (m_empty)
			return;
		if (m_needToUpload) {
			m_needToUpload = false;
			qSwap(m_interm, m_showing);
			if (m_size[m_showing].isEmpty())
				return;
			upload(m_size[m_showing], m_buffer[m_showing].data(), 0);
		}
		const int idx = 0;
		shader->bind();
		shader->setUniformValue("tex", 0);
		shader->setUniformValue("dxdy", dx(idx), dy(idx));
		if (renderer()->style().has_shadow) {
			shader->setUniformValue("shadow_color", renderer()->style().shadow_color);
			const QPointF pos = renderer()->style().shadow_offset*renderer()->scale();
//			qDebug() << pos;
			shader->setUniformValue("shadow_offset", pos);
		} else {
			shader->setUniformValue("shadow_color", 0.f, 0.f, 0.f, 0.f);
			shader->setUniformValue("shadow_offset", 0.f, 0.f);
		}
		func->glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture(idx));
		float textureCoords[] = {
			0.f, 0.f,						sub_x(idx), 0.f,
			sub_x(idx), sub_y(idx),			0.f, sub_y(idx)
		};
		const QPointF pos = renderer()->posHint() + (renderer()->letterboxHint() ? rpos : fpos);
		const QSizeF size = this->size(idx);
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
	QByteArray m_buffer[3];
	OsdRenderer *m_renderer;
	int m_drawing = 0, m_interm = 1, m_showing = 2;
	QSize m_size[3];
};


struct Overlay::Data {
	QGLWidget *ctx;
	QLinkedList<QLinkedList<ScreenOsdWrapper*> > osds;
	VideoScreen *screen;
	QRect renderable;
	QRect frame;
	bool pending = false;
	QGLShaderProgram *shader;
};

Overlay::Overlay(VideoScreen *screen)
: d(new Data) {
	Q_ASSERT(screen);
	d->screen = screen;
	d->ctx = new QGLWidget(nullptr, screen);
	d->ctx->makeCurrent();
	initializeGLFunctions(d->ctx->context());
	d->shader = new QGLShaderProgram(d->ctx->context());
	d->shader->addShaderFromSourceCode(QGLShader::Fragment,
		"uniform sampler2D tex;"
		"uniform vec2 dxdy;"
		"uniform vec2 shadow_offset;"
		"uniform vec4 shadow_color;"
		"void main() {"
		"	vec4 top = texture2D(tex, gl_TexCoord[0].xy);"
		"	float alpha = texture2D(tex, gl_TexCoord[0].xy - dxdy*shadow_offset).a;"
		"	gl_FragColor = top + alpha*shadow_color*(1.0 - top.a);"
		"}");
	d->shader->link();
	d->ctx->doneCurrent();
}

Overlay::~Overlay() {
	d->ctx->makeCurrent();
	for (auto wrappers : d->osds) {
		for (auto wrapper : wrappers) {
			wrapper->free();
			delete wrapper;
		}
	}
	delete d->shader;
	d->ctx->doneCurrent();
	delete d->ctx;
	delete d;
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
//	d->ctx->makeCurrent();
	for (auto wrappers : d->osds) {
		for (auto wrapper : wrappers) {
			if (wrapper->renderer()->setArea(renderable.size(), frame.size()))
				wrapper->cache();
		}
	}
//	d->ctx->doneCurrent();
	d->pending = false;
	d->screen->redraw();
}

void Overlay::update() {
	auto osd = qobject_cast<OsdRenderer*>(sender());
	if (osd && !d->pending) {
		auto wrapper = osd->wrapper();
		wrapper->cache();
		d->screen->redraw();
	}
}

void Overlay::add(OsdRenderer *osd) {
	d->ctx->makeCurrent();
	OsdRenderer *next = osd;
	d->osds.push_back(QLinkedList<ScreenOsdWrapper*>());
	auto &list = d->osds.last();
	while (next) {
		next->setArea(d->renderable.size(), d->frame.size());
		connect(next, SIGNAL(needToRerender()), this, SLOT(update()));
		auto wrapper = new ScreenOsdWrapper(next);
		wrapper->alloc();
		list.push_back(wrapper);
		next = next->next();
	}
	d->ctx->doneCurrent();
	connect(osd, SIGNAL(destroyed()), this, SLOT(onOsdDeleted()));
}

void Overlay::onOsdDeleted() {
	take(static_cast<OsdRenderer*>(sender()));
}

OsdRenderer *Overlay::take(OsdRenderer *osd) {
	d->ctx->makeCurrent();
	for (auto it = d->osds.begin(); it != d->osds.end(); ++it) {
		if (it->first()->renderer() == osd) {
			qDeleteAll(*it);
			d->osds.erase(it);
			break;
		}
	}
	d->ctx->doneCurrent();
	return osd;
}

void Overlay::render(QPainter *painter) {
	for (auto wrappers : d->osds) {
		QPointF o(0.0, 0.0);
		for (auto wrapper : wrappers) {
			if (!wrapper->isEmpty())  {
				auto osd = wrapper->renderer();
				osd->render(painter, osd->posHint() + o);
				o.ry() += osd->height();
			}
		}
	}
}

void Overlay::renderToScreen() {
	const QPointF rpos = d->renderable.topLeft();
	const QPointF fpos = d->frame.topLeft();
	for (auto wrappers : d->osds) {
		QPointF o(0.0, 0.0);
		for (auto wrapper : wrappers) {
			if (!wrapper->isEmpty()) {
				wrapper->render(this, rpos + o, fpos + o, d->shader);
				o.ry() += wrapper->renderer()->height();
			}
		}
	}
}
