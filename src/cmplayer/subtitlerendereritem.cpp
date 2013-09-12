#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"
#include "subtitlemodel.hpp"
#include "subtitlestyle.hpp"
#include "subtitledrawer.hpp"
#include "subtitlerenderingthread.hpp"
#include "openglcompat.hpp"

struct SubtitleRendererItem::Data {
	SubtitleStyle style;
	SubtitleDrawer drawer;
	bool redraw = false, selecting = false;
	int loc_tex = 0, loc_shadowColor = 0, loc_shadowOffset = 0, loc_dxdy = 0;
	QPointF shadowOffset = {0, 0};
	QMap<QString, int> langMap;
	int language_priority(const SubtitleComponent &comp) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(comp.language(), -1);
	}
	SubtitleRenderingThread *renderer = nullptr;
	SubtitleRenderingThread::RenderTarget order;
	SubtitleRenderingThread::Picture pic = {order};
	QVector<SubtitleComponentModel*> models;
	OpenGLTexture texture;
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
	updateAlignment();	updateStyle();
	d->texture.format = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV};
}

SubtitleRendererItem::~SubtitleRendererItem() {
	qDeleteAll(d->models);
	delete d->renderer;
	delete d;
}

void SubtitleRendererItem::rerender() {
	if (!m_hidden && !m_compempty && m_ms > 0 && d->renderer)
		d->renderer->rerender(m_ms - m_delay, m_fps);

}

const RichTextDocument &SubtitleRendererItem::text() const {
	return d->pic.text;
}

void SubtitleRendererItem::setLetterboxHint(bool hint) {
	if (_Change(m_letterbox, hint) && m_screen != boundingRect() && d->renderer)
		d->renderer->setArea(drawArea(), dpr());
}

void SubtitleRendererItem::setScreenRect(const QRectF &screen) {
	if (_Change(m_screen, screen) && !m_letterbox && d->renderer) {
		d->renderer->setArea(drawArea(), dpr());
		setGeometryDirty();
	}
}

void SubtitleRendererItem::setPriority(const QStringList &priority) {
	for (int i=0; i< priority.size(); ++i)
		d->langMap[priority[i]] = priority.size()-i;
}

void SubtitleRendererItem::setStyle(const SubtitleStyle &style) {
	d->style = style;
	updateStyle();
}

void SubtitleRendererItem::updateStyle() {
	d->drawer.setStyle(d->style);
	if (d->renderer)
		d->renderer->setDrawer(d->drawer);
	setGeometryDirty();
}

void SubtitleRendererItem::updateAlignment() {
	d->drawer.setAlignment(m_alignment);
	if (d->renderer)
		d->renderer->setDrawer(d->drawer);
}

void SubtitleRendererItem::setTopAlignment(bool top) {
	if (_Change(m_top, top)) {
		if (_Change(m_alignment, Qt::AlignHCenter | (m_top ? Qt::AlignTop : Qt::AlignBottom)))
			updateAlignment();
		setPos(1.0 - m_pos);
	}
}

const SubtitleStyle &SubtitleRendererItem::style() const {
	return d->style;
}

//const RichTextDocument &SubtitleRendererItem::text() const {
//	return d->drawer.text();
//}

double SubtitleRendererItem::scale(const QRectF &area) const {
	const auto policy = d->style.font.scale;
	double px = d->style.font.size;
	if (policy == SubtitleStyle::Font::Scale::Diagonal)
		px *= _Diagonal(area.size());
	else if (policy == SubtitleStyle::Font::Scale::Width)
		px *= area.width();
	else
		px *= area.height();
	return px/d->style.font.height();
}

void SubtitleRendererItem::unload() {
	qDeleteAll(d->models);
	d->models.clear();
	delete d->renderer;
	d->renderer = nullptr;
	d->order.clear();
	m_loaded.clear();
	clear();
	m_compempty = true;
	emit modelsChanged(models());
}

void SubtitleRendererItem::clear() {
	d->pic = SubtitleRenderingThread::Picture(d->order);
	setVisible(false);
}

void SubtitleRendererItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_tex = program->uniformLocation("tex");
	d->loc_dxdy = program->uniformLocation("dxdy");
	d->loc_shadowColor = program->uniformLocation("shadowColor");
	d->loc_shadowOffset = program->uniformLocation("shadowOffset");
}

void SubtitleRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_tex, 0);
	program->setUniformValue(d->loc_shadowColor, d->style.shadow.color);
	program->setUniformValue(d->loc_shadowOffset, d->shadowOffset);
	program->setUniformValue(d->loc_dxdy, 1.0/(d->pic.image.width()), 1.0/(d->pic.image.height()));
	auto f = QOpenGLContext::currentContext()->functions();
	f->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture(0));
}

void SubtitleRendererItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	set(tp, QRectF(d->drawer.pos(d->pic.size, drawArea()), d->pic.size), QRectF(0, 0, 1, 1));
}

void SubtitleRendererItem::initializeTextures() {
	if (!d->pic.image.isNull()) {
		d->texture.id = texture(0);
		d->texture.width = d->pic.image.width();
		d->texture.height = d->pic.image.height();
		d->texture.allocate(GL_LINEAR, d->pic.image.bits());
	}
	setGeometryDirty();
}

void SubtitleRendererItem::beforeUpdate() {
	if (d->redraw) {
		initializeTextures();
		setGeometryDirty();
		d->redraw = false;
	}
}

void SubtitleRendererItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	TextureRendererItem::geometryChanged(newGeometry, oldGeometry);
	if (m_letterbox) {
		setGeometryDirty();
		if (d->renderer)
			d->renderer->setArea(drawArea(), dpr());
	}
}

const char *SubtitleRendererItem::fragmentShader() const {
	const char *shader = (R"(
		uniform sampler2D tex;
		uniform vec2 dxdy;
		uniform vec2 shadowOffset;
		uniform vec4 shadowColor;
		varying highp vec2 qt_TexCoord;
		void main() {
			vec4 top = texture2D(tex, qt_TexCoord);
			float alpha = texture2D(tex, qt_TexCoord - shadowOffset).a;
			gl_FragColor = top + alpha*shadowColor*(1.0 - top.a);
		}
	)");
	return shader;
}

const Margin &SubtitleRendererItem::margin() const {
	return d->drawer.margin();
}

void SubtitleRendererItem::setMargin(double top, double bottom, double right, double left) {
	Margin margin;
	margin.top = top; margin.bottom = bottom; margin.right = right; margin.left = left;
	d->drawer.setMargin(margin);
	setGeometryDirty();
	update();
}

void SubtitleRendererItem::setHidden(bool hidden) {
	if (_Change(m_hidden, hidden))
		setVisible(!m_hidden && !d->pic.image.isNull());
}

void SubtitleRendererItem::render(int ms) {
	m_ms = ms;
	if (!m_hidden && !m_compempty && ms > 0 && d->renderer)
		d->renderer->render(ms - m_delay, m_fps);
}

bool SubtitleRendererItem::removeInOrder(const SubtitleComponent *comp) {
	for (auto it = d->order.begin(); it!= d->order.end(); ++it) {
		if (*it == comp) {
			d->order.erase(it);
			return true;
		}
	}
	return false;
}

void SubtitleRendererItem::deselect(int idx) {
	if (idx < 0) {
		d->order.clear();
		for (auto &loaded : m_loaded)
			loaded.selection() = false;
	} else if (_InRange(0, idx, m_loaded.size()-1) && m_loaded[idx].isSelected()) {
		removeInOrder(&m_loaded[idx].component());
		m_loaded[idx].selection() = false;
	} else
		return;
	if (!d->selecting)
		applySelection();
}

void SubtitleRendererItem::applySelection() {
	m_compempty = true;
	qDeleteAll(d->models);
	d->models.clear();
	d->models.reserve(d->order.size());
	for (auto comp : d->order) {
		d->models.append(new SubtitleComponentModel(comp, this));
		if (m_compempty && !comp->isEmpty())
			m_compempty = false;
	}
	emit modelsChanged(d->models);
	delete d->renderer;
	d->renderer = nullptr;
	if (!m_compempty) {
		d->renderer = new SubtitleRenderingThread(d->order, this);
		d->renderer->setDrawer(d->drawer);
		d->renderer->setArea(drawArea(), dpr());
		d->renderer->start();
		d->renderer->render(m_ms, m_fps);
	} else
		setVisible(false);
}

void SubtitleRendererItem::select(int idx) {
	if (idx < 0) {
		d->order.clear();
		const bool wasSelecting = d->selecting;
		if (!wasSelecting)
			d->selecting = true;
		for (int i=0; i<m_loaded.size(); ++i) {
			m_loaded[i].selection() = false;
			select(i);
		}
		if (!wasSelecting) {
			d->selecting = false;
			applySelection();
		}
	} else if (_InRange(0, idx, m_loaded.size()-1) && !m_loaded[idx].isSelected()) {
		auto &loaded = m_loaded[idx];
		removeInOrder(&loaded.component());
		loaded.selection() = true;
		d->order.prepend(&loaded.component());
		for (auto it = d->order.begin(); it != d->order.end(); ) {
			const SubtitleComponent *&prev = *it;
			if (++it == d->order.end())
				break;
			const SubtitleComponent *&next = *it;
			if (d->language_priority(*prev) >= d->language_priority(*next))
				break;
			qSwap(prev, next);
		}
		if (!d->selecting)
			applySelection();
	}
}

bool SubtitleRendererItem::load(const Subtitle &subtitle, bool select) {
	const int idx = m_loaded.size();
	if (subtitle.isEmpty())
		return false;
	for (int i=0; i<subtitle.size(); ++i)
		m_loaded.append(LoadedSubtitle(subtitle[i]));
	if (select) {
		d->selecting = true;
		for (int i=m_loaded.size()-1; i>=idx; --i) {
			if (select)
				this->select(i);
		}
		d->selecting = false;
		applySelection();
	}
	return true;
}

int SubtitleRendererItem::start(int time) const {
	int ret = -1;
	for (auto it = d->order.begin(); it != d->order.end(); ++it) {
		const auto &comp = *(*it);
		const auto sIt = comp.start(time - m_delay, m_fps);
		if (sIt != comp.end())
			ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(sIt.key(), m_fps) : sIt.key());
	}
	return ret;
}

int SubtitleRendererItem::finish(int time) const {
	int ret = -1;
	for (auto it = d->order.begin(); it != d->order.end(); ++it) {
		const auto &comp = *(*it);
		const auto sIt = comp.finish(time - m_delay, m_fps);
		if (sIt != comp.end()) {
			const int t = comp.isBasedOnFrame() ? comp.msec(sIt.key(), m_fps) : sIt.key();
			ret = ret == -1 ? t : qMin(ret, t);
		}
	}
	return ret;
}

int SubtitleRendererItem::current() const {
	int time = -1;
	Q_ASSERT(d->pic.its.size() == d->order.size());
	for (int i=0; i<d->order.size(); ++i) {
		const auto it = d->pic.its[i];
		if (it != d->order[i]->end() && it->hasWords()) {
			if (time < 0)
				time = it.key();
			else if (it.key() > time)
				time = it.key();
		}
	}
	return time;
}

int SubtitleRendererItem::previous() const {
	int time = -1;
	for (int i=0; i<d->order.size(); ++i) {
		auto it = d->pic.its[i];
		if (it != d->order[i]->end()) {
			while (it != d->order[i]->begin()) {
				if ((--it)->hasWords()) {
					if (time < 0)
						time = it.key();
					else if (it.key() > time)
						time = it.key();
					break;
				}
			}
		}
	}
	return time;
}

int SubtitleRendererItem::next() const {
	int time = -1;
	for (int i=0; i<d->order.size(); ++i) {
		auto it = d->pic.its[i];
		if (it != d->order[i]->end()) {
			while (++it != d->order[i]->end()) {
				if (it->hasWords()) {
					if (time < 0)
						time = it.key();
					else if (it.key() < time)
						time = it.key();
					break;
				}
			}
		}
	}
	return time;
}

void SubtitleRendererItem::setLoaded(const QList<LoadedSubtitle> &loaded) {
	unload();
	m_loaded = loaded;
	for (const auto &loaded : m_loaded) {
		if (loaded.isSelected())
			d->order.prepend(&loaded.component());
	}
	qSort(d->order.begin(), d->order.end(), [this] (const SubtitleComponent *lhs, const SubtitleComponent *rhs) {
		return d->language_priority(*lhs) > d->language_priority(*rhs);
	});
	applySelection();
}

QVector<SubtitleComponentModel*> SubtitleRendererItem::models() const {
	return d->models;
}

void SubtitleRendererItem::customEvent(QEvent *event) {
	if (event->type() == (int)SubtitleRenderingThread::Prepared) {
		auto e = static_cast<DataEvent2<SubtitleRenderingThread*, SubtitleRenderingThread::Picture>*>(event);
		if (e->data1() == d->renderer) {
			d->pic = e->data2();
			if (!d->pic.image.isNull()) {
				d->redraw = true;
				d->shadowOffset.rx() = d->pic.shadow.x()/(double)d->pic.size.width();
				d->shadowOffset.ry() = d->pic.shadow.y()/(double)d->pic.size.height();
				update();
			}
			setVisible(!m_hidden && !d->pic.image.isNull());
			for (int i=0; i<d->models.size(); ++i)
				d->models[i]->setCurrentCaption(&(*d->pic.its[i]));
		}
	}
}
