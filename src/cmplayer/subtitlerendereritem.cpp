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
	bool selecting = false;
	bool redraw = false;
	QPointF shadowOffset = {0, 0};
	QMap<QString, int> langMap;
	int language_priority(const SubtitleComponent &comp) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(comp.language(), -1);
	}
	SubtitleRenderingThread *renderer = nullptr;
	SubtitleRenderingThread::RenderTarget order;
	SubtitleRenderingThread::Picture pic{order};
	QVector<SubtitleComponentModel*> models;
	OpenGLTexture texture;
};

class SubtitleRendererShader : public TextureRendererShader {
public:
	SubtitleRendererShader(const SubtitleRendererItem *item)
	: TextureRendererShader(item, false) { }
	const char *fragmentShader() const {
		const char *shader = (R"(
			uniform sampler2D tex;
			uniform vec2 dxdy;
			uniform vec2 shadowOffset;
			uniform vec4 shadowColor;
			varying highp vec2 texCoord;
			void main() {
				vec4 top = texture2D(tex, texCoord);
				float alpha = texture2D(tex, texCoord - shadowOffset).a;
				gl_FragColor = top + alpha*shadowColor*(1.0 - top.a);
			}
		)");
		return shader;
	}
	void link(QOpenGLShaderProgram *prog) override {
		loc_tex = prog->uniformLocation("tex");
		loc_dxdy = prog->uniformLocation("dxdy");
		loc_shadowColor = prog->uniformLocation("shadowColor");
		loc_shadowOffset = prog->uniformLocation("shadowOffset");
	}
	void bind(QOpenGLShaderProgram *prog) override {
		auto d = static_cast<const SubtitleRendererItem*>(item())->d;
		prog->setUniformValue(loc_tex, 0);
		prog->setUniformValue(loc_shadowColor, d->style.shadow.color);
		prog->setUniformValue(loc_shadowOffset, d->shadowOffset);
		prog->setUniformValue(loc_dxdy, 1.0/(d->pic.image.width()), 1.0/(d->pic.image.height()));
	}
private:
	int loc_tex = -1, loc_shadowColor = -1, loc_shadowOffset = -1, loc_dxdy = -1, loc_vMatrix = -1;
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(parent), d(new Data) {
	d->renderer = new SubtitleRenderingThread(this);
	d->renderer->start();
	updateAlignment();
	updateStyle();
}

SubtitleRendererItem::~SubtitleRendererItem() {
	unload();
	d->renderer->quit();
	d->renderer->wait(50000);
	_Delete(d->renderer);
	delete d;
}

void SubtitleRendererItem::rerender() {
	if (!m_hidden && !m_compempty && m_ms > 0)
		d->renderer->rerender(m_ms - m_delay, m_fps);
}

TextureRendererShader *SubtitleRendererItem::createShader() const {
	return new SubtitleRendererShader(this);
}

void SubtitleRendererItem::initializeGL() {
	TextureRendererItem::initializeGL();
	d->texture.format = OpenGLCompat::textureFormat(GL_BGRA);
	d->texture.generate();
	setRenderTarget(d->texture);
}

void SubtitleRendererItem::finalizeGL() {
	TextureRendererItem::finalizeGL();
	d->texture.delete_();
}

const RichTextDocument &SubtitleRendererItem::text() const {
	return d->pic.text;
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
	d->renderer->setDrawer(d->drawer);
	setGeometryDirty();
}

void SubtitleRendererItem::updateAlignment() {
	d->drawer.setAlignment(m_alignment);
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
	d->renderer->clear(true);
	qDeleteAll(d->models);
	d->models.clear();
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

void SubtitleRendererItem::getCoords(QRectF &vertices, QRectF &) {
	vertices = QRectF(d->drawer.pos(d->pic.size, rect()), d->pic.size);
}

void SubtitleRendererItem::prepare(QSGGeometryNode *node) {
	if (d->redraw) {
		if (!d->pic.image.isNull()) {
			d->texture.width = d->pic.image.width();
			d->texture.height = d->pic.image.height();
			d->texture.allocate(GL_LINEAR, d->pic.image.bits());
		}
		setGeometryDirty();
		d->redraw = false;
		node->markDirty(QSGNode::DirtyMaterial);
	}
}

void SubtitleRendererItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	d->renderer->setArea(rect(), dpr());
	TextureRendererItem::geometryChanged(newGeometry, oldGeometry);
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
	if (!m_hidden && !m_compempty && ms > 0)
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
	d->renderer->clear();
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
	d->renderer->clear();
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
	if (!m_compempty) {
		d->renderer->reset(d->order);
		d->renderer->render(m_ms, m_fps);
	} else {
		setVisible(false);
	}
}

void SubtitleRendererItem::select(int idx) {
	if (idx < 0) {
		d->renderer->clear();
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
		d->renderer->clear();
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
	if (subtitle.isEmpty())
		return false;
	const int idx = m_loaded.size();
	d->renderer->clear();
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
	} else
		d->renderer->reset(d->order);
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
	d->renderer->clear();
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
		getAllData(event, d->pic);
		if (!d->pic.image.isNull()) {
			d->shadowOffset.rx() = d->pic.shadow.x()/(double)d->pic.size.width();
			d->shadowOffset.ry() = d->pic.shadow.y()/(double)d->pic.size.height();
			d->redraw = true;
			update();
		}
		setVisible(!m_hidden && !d->pic.image.isNull());
		for (int i=0; i<d->models.size() && i<d->pic.its.size(); ++i)
			d->models[i]->setCurrentCaption(&(*d->pic.its[i]));
	}
}
