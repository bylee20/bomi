#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"
#include "subtitlemodel.hpp"
#include "subtitlestyle.hpp"
#include "subtitledrawer.hpp"
#include "subtitlerenderingthread.hpp"
#include "openglcompat.hpp"

struct SubtitleRendererItem::Data {
	SubtitleRendererItem *p = nullptr;
	SubtitleStyle style;
	SubtitleDrawer drawer;
	bool selecting = false;
	bool redraw = false;
	QPointF shadowOffset = {0, 0};
	QMap<QString, int> langMap;
	QMutex mutex;
	QWaitCondition wait;
	int language_priority(const SubComp &comp) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(comp.language(), -1);
	}
	QList<SubCompObject> selection;
	OpenGLTexture texture;
	template<typename Func>
	void apply(Func func) {
		mutex.lock();
		for (const auto &obj : selection)
			if (obj.thread)
				func(obj.thread);
		mutex.unlock();
		wait.wakeAll();
	}
	void updateDrawer() {
		apply([this] (SubCompThread *thread) { thread->setDrawer(drawer); });
	}
	void clearSelection() {
		for (auto &obj : selection) {
			delete obj.thread;
			delete obj.model;
		}
		selection.clear();
	}

	SubCompObject object(const SubComp *comp) {
		SubCompObject obj;
		if (!comp)
			return obj;
		obj.comp = comp;
		obj.thread = new SubCompThread(&mutex, &wait, comp, p);
		obj.model = new SubtitleComponentModel(comp, p);
		obj.thread->setDrawer(drawer);
		obj.thread->setArea(p->rect(), p->dpr());
		obj.thread->start();
		return obj;
	}

	SubCompObject take(const SubComp *comp) {
		auto it = std::find_if(selection.begin(), selection.end()
			, [comp] (const SubCompObject &obj) { return obj.comp == comp; });
		if (it == selection.end())
			return SubCompObject();
		auto ret = *it;
		selection.erase(it);
		return ret;
	}
	SubCompObject *find(const SubComp *comp) {
		auto it = std::find_if(selection.begin(), selection.end(), [comp] (const SubCompObject &obj) { return obj.comp == comp; });
		return it != selection.cend() ? &(*it) : nullptr;
	}

	bool isSelected(const SubComp *comp) {
		auto it = _FindIf(selection, [comp] (const SubCompObject &obj) { return obj.comp == comp; });
		return it != selection.cend();
	}
	void sort() {
		qSort(selection.begin(), selection.end(), [this] (const SubCompObject &lhs, const SubCompObject &rhs) {
			return language_priority(*lhs.comp) > language_priority(*rhs.comp);
		});
	}

	RichTextDocument text;
};

class SubtitleRendererShader : public TextureRendererShader {
public:
	SubtitleRendererShader(const SubtitleRendererItem *item)
	: TextureRendererShader(item, false) { }
	const char *fragmentShader() const {
		const char *shader = (R"(
			uniform sampler2D tex;
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
		loc_shadowColor = prog->uniformLocation("shadowColor");
		loc_shadowOffset = prog->uniformLocation("shadowOffset");
	}
	void bind(QOpenGLShaderProgram *prog) override {
		auto d = static_cast<const SubtitleRendererItem*>(item())->d;
		prog->setUniformValue(loc_tex, 0);
		prog->setUniformValue(loc_shadowColor, d->style.shadow.color);
		prog->setUniformValue(loc_shadowOffset, d->shadowOffset);
	}
private:
	int loc_tex = -1, loc_shadowColor = -1, loc_shadowOffset = -1;
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(parent), d(new Data) {
	d->p = this;
	updateAlignment();
	updateStyle();
}

SubtitleRendererItem::~SubtitleRendererItem() {
	unload();
	delete d;
}

void SubtitleRendererItem::rerender() {
	if (!m_hidden && !m_compempty && m_ms > 0)
		d->apply([this] (SubCompThread *thread) { thread->rerender(m_ms - m_delay, m_fps); });
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
	return d->text;
//	return d->pic.text;
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
	d->updateDrawer();
	setGeometryDirty();
}

void SubtitleRendererItem::updateAlignment() {
	d->drawer.setAlignment(m_alignment);
	d->updateDrawer();
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
	d->clearSelection();
	qDeleteAll(m_loaded);
	m_loaded.clear();
	clear();
	m_compempty = true;
	emit modelsChanged(models());
}

void SubtitleRendererItem::clear() {
	setVisible(false);
}

void SubtitleRendererItem::getCoords(QRectF &vertices, QRectF &) {
	vertices = QRectF(d->drawer.pos(d->texture.size(), rect()), d->texture.size());
}

void SubtitleRendererItem::prepare(QSGGeometryNode *node) {
	if (d->redraw) {
		d->texture.width = 0;
		d->texture.height = 0;
		for (const auto &obj : d->selection) {
			d->texture.width = qMax(d->texture.width, obj.picture.image().width());
			d->texture.height += obj.picture.image().height();
		}
		if (!d->texture.size().isEmpty()) {
			d->texture.allocate(GL_LINEAR);
			int y = 0;
			d->shadowOffset = d->selection[0].picture.shadowOffset();
			for (const auto &obj : d->selection) {
				auto &image = obj.picture.image();
				if (!image.isNull())
					d->texture.upload(0, y, image.size(), image.bits());
				y += image.height();
				if (qAbs(d->shadowOffset.x()) < qAbs(obj.picture.shadowOffset().x()))
					d->shadowOffset = obj.picture.shadowOffset();
			}
			d->shadowOffset.rx() /= (double)d->texture.width;
			d->shadowOffset.ry() /= (double)d->texture.height;
			setGeometryDirty();
			node->markDirty(QSGNode::DirtyMaterial);
		}
		d->redraw = false;
	}
	setVisible(!m_hidden && !d->texture.size().isEmpty());
}

void SubtitleRendererItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	d->apply([this] (SubCompThread *thread) { thread->setArea(rect(), dpr()); });
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
		setVisible(!m_hidden && !d->texture.size().isEmpty());
}

void SubtitleRendererItem::render(int ms) {
	m_ms = ms;
	if (!m_hidden && !m_compempty && ms > 0)
		d->apply([this] (SubCompThread *thread) { thread->render(m_ms - m_delay, m_fps); });
}

void SubtitleRendererItem::deselect(int idx) {
	if (idx < 0) {
		d->clearSelection();
		for (auto &loaded : m_loaded)
			loaded->selection() = false;
	} else if (_InRange(0, idx, m_loaded.size()-1) && m_loaded[idx]->isSelected()) {
		delete d->take(&m_loaded[idx]->component()).thread;
		m_loaded[idx]->selection() = false;
	} else
		return;
	if (!d->selecting)
		applySelection();
}

void SubtitleRendererItem::applySelection() {
	m_compempty = true;
	QVector<SubtitleComponentModel*> models;
	for (const auto &obj : d->selection) {
		models << obj.model;
		if (m_compempty && !obj.comp->isEmpty())
			m_compempty = false;
	}
	emit modelsChanged(models);
	if (!m_compempty)
		render(m_ms);
	else
		setVisible(false);
}

void SubtitleRendererItem::select(int idx) {
	bool sort = false;
	if (idx < 0) {
		for (auto &loaded : m_loaded) {
			auto comp = &loaded->component();
			if (!d->isSelected(comp)) {
				d->selection.push_front(d->object(comp));
				sort = true;
			}
		}
	} else if (_InRange(0, idx, m_loaded.size()-1) && !m_loaded[idx]->isSelected()) {
		sort = true;
		auto &loaded = m_loaded[idx];
		auto obj = d->take(&loaded->component());
		if (!obj.comp)
			obj = d->object(&loaded->component());
		loaded->selection() = true;
		d->selection.push_front(obj);
	}
	if (sort) {
		d->sort();
		if (!d->selecting)
			applySelection();
	}
}

bool SubtitleRendererItem::load(const Subtitle &subtitle, bool select) {
	if (subtitle.isEmpty())
		return false;
	const int idx = m_loaded.size();
	for (int i=0; i<subtitle.size(); ++i)
		m_loaded.append(new LoadedSubtitle(subtitle[i]));
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
	for (const SubCompObject &obj : d->selection) {
		const auto &comp = *obj.comp;
		const auto it = comp.start(time - m_delay, m_fps);
		if (it != comp.end())
			ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(it.key(), m_fps) : it.key());
	}
	return ret;
}

int SubtitleRendererItem::finish(int time) const {
	int ret = -1;
	for (const SubCompObject &obj : d->selection) {
		const auto &comp = *obj.comp;
		const auto it = comp.finish(time - m_delay, m_fps);
		if (it != comp.end()) {
			const int t = comp.isBasedOnFrame() ? comp.msec(it.key(), m_fps) : it.key();
			ret = ret == -1 ? t : qMin(ret, t);
		}
	}
	return ret;
}

int SubtitleRendererItem::current() const {
	int time = -1;
	for (const SubCompObject &obj : d->selection) {
		const auto it = obj.picture.iterator();
		if (it != obj.comp->cend() && it->hasWords()) {
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
	for (const SubCompObject &obj : d->selection) {
		auto it = obj.picture.iterator();
		if (it != obj.comp->cend()) {
			while (it != obj.comp->cbegin()) {
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
	for (const SubCompObject &obj : d->selection) {
		auto it = obj.picture.iterator();
		if (it != obj.comp->cend()) {
			while (++it != obj.comp->cend()) {
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
	m_loaded.reserve(loaded.size());
	for (auto &l : loaded)
		m_loaded.push_back(new LoadedSubtitle(l));
	for (const auto &loaded : m_loaded) {
		if (loaded->isSelected())
			d->selection.push_front(d->object(&loaded->component()));
	}
	d->sort();
	applySelection();
}

QVector<SubtitleComponentModel*> SubtitleRendererItem::models() const {
	QVector<SubtitleComponentModel*> models;
	for (const auto &obj : d->selection)
		models << obj.model;
	return models;
}

void SubtitleRendererItem::customEvent(QEvent *event) {
	if (event->type() == (int)SubCompThread::Prepared) {
		auto pic = getData<SubCompPicture>(event);
		auto obj = d->find(pic.component());
		if (obj) {
			obj->picture = pic;
			obj->model->setCurrentCaption(&(*pic.iterator()));
		}
		d->redraw = true;
		update();
	}
}
