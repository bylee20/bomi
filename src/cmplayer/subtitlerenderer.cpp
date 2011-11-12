#include "subtitlerenderer.hpp"
#include <QtCore/QLinkedList>
#include "info.hpp"
#include "mrl.hpp"
#include "pref.hpp"
#include "subtitlemodel.hpp"
#include "osdstyle.hpp"
#include "subtitleview.hpp"
#include <QtCore/QDebug>

SubtitleRenderer::Render::Render(const Comp &comp) {
	this->comp = &comp;
	this->model = new SubtitleComponentModel(this->comp);
	this->prev = this->comp->end();
}

SubtitleRenderer::Render::~Render() {
	delete this->model;
}

struct SubtitleRenderer::Data {
	SubtitleView *view;
	TextOsdRenderer *osd;
	double fps;
	int delay, ms;
	double pos;
	bool visible, empty, top;
	QList<Loaded> loaded;
	RenderList order;
	bool selecting;
	void set_model_list() {
		if (!view)
			return;
		QVector<SubtitleComponentModel*> list;
		list.reserve(order.size());
		RenderList::const_iterator it = order.begin();
		for (; it!=order.end(); ++it) {
			const Render *render = *it;
			list.push_back(render->model);
			if (!render->comp->isEmpty())
				empty = false;
		}
		view->setModel(list);
	}
	void reset_prev() {
		RenderList::const_iterator it = order.begin();
		for (; it!=order.end(); ++it) {
			Render *r = *it;
			r->prev = r->comp->end();
		}
	}
	QMap<QString, int> langMap;
	int language_priority(const Render *r) const {
		return langMap.value(r->comp->language().id(), -1);
	}
	void reset_lang_map() {
		const QStringList priority = Pref::get().sub_priority;
		for (int i=0; i< priority.size(); ++i)
			langMap[priority[i]] = priority.size()-i;
	}
};

SubtitleRenderer::SubtitleRenderer(): d(new Data) {
	d->top = d->selecting = false;
	d->empty = d->visible = true;
	d->osd = new TextOsdRenderer(Qt::AlignBottom | Qt::AlignCenter);
//	d->osd->setLetterboxHint(true);
	d->fps = 30;
	d->ms = d->delay = 0;
	d->pos = 1.0;
	d->view = 0;
	d->reset_lang_map();
}

SubtitleRenderer::~SubtitleRenderer() {
	delete d;
}

QWidget *SubtitleRenderer::view(QWidget *parent) const {
	if (!d->view) {
		d->view = new SubtitleView(parent);
		d->set_model_list();
	}
	return d->view;
}

TextOsdRenderer *SubtitleRenderer::osd() const {
	return d->osd;
}

void SubtitleRenderer::setFrameRate(double fps) {
	if (d->fps != fps) {
		d->fps = fps;
		d->reset_prev();
	}
}

void SubtitleRenderer::render(int ms) {
	d->ms = ms;
	if (!d->visible || d->empty || ms == 0)
		return;
	bool changed = false;
	RenderList::const_iterator o = d->order.begin();
	for (; o != d->order.end(); ++o) {
		Render &render = **o;
		CompIt it = render.comp->start(ms - d->delay, d->fps);
		if (it != render.prev) {
			render.prev = it;
			render.model->setCurrentNode(&(*it));
			changed = true;
		}
	}
	if (changed) {
		RichString text;
		for (o = d->order.begin(); o != d->order.end(); ++o) {
			const Render &render = **o;
			if (render.prev != render.comp->end()) {
				text.merge(render.prev->text);
			}
		}
		d->osd->showText(text);
	}
}

void SubtitleRenderer::setVisible(bool visible) {
	if (visible == d->visible)
		return;
	d->visible = visible;
	if (d->visible)
		render(d->ms);
	else
		clear();
}

void SubtitleRenderer::clear() {
	d->reset_prev();
	d->osd->clear();
}

double SubtitleRenderer::frameRate() const {
	return d->fps;
}

int SubtitleRenderer::delay() const {
	return d->delay;
}

void SubtitleRenderer::unload() {
	qDeleteAll(d->order);
	d->order.clear();
	d->loaded.clear();
	d->set_model_list();
	d->empty = true;
	clear();
}

const QList<SubtitleRenderer::Loaded> &SubtitleRenderer::loaded() const {
	return d->loaded;
}

void SubtitleRenderer::select(int idx, bool selected) {
	if (0 <= idx && idx < d->loaded.size() && d->loaded[idx].isSelected() != selected) {
		d->loaded[idx].m_selected = selected;
		RenderList::iterator o = d->order.begin();
		const Comp *comp = &d->loaded[idx].m_comp;
		Render *render = 0;
		for (; o!= d->order.end(); ++o) {
			if ((*o)->comp == comp) {
				render = *o;
				d->order.erase(o);
				render->prev = comp->end();
				break;
			}
		}
		if (selected) {
			if (!render)
				render = new Render(*comp);
			d->order.prepend(render);
			o = d->order.begin();
			while (o != d->order.end()) {
				Render *&prev = *o;
				if (++o == d->order.end())
					break;
				Render *&next = *o;
				if (d->language_priority(prev) >= d->language_priority(next))
					break;
				qSwap(prev, next);
			}
		}
		if (!d->selecting)
			applySelection();
	}
}

void SubtitleRenderer::applySelection() {
	d->empty = true;
	d->set_model_list();
	if (d->empty)
		clear();
	else
		this->render(d->ms);
}

bool SubtitleRenderer::load(const QString &fileName, const QString &enc, bool select) {
	const int idx = d->loaded.size();
	Subtitle sub;
	if (!sub.load(fileName, enc))
		return false;
	for (int i=0; i<sub.size(); ++i) {
		d->loaded.append(Loaded(sub[i]));
	}
	if (select) {
		d->selecting = true;
		for (int i=d->loaded.size()-1; i>=idx; --i) {
			this->select(i, select);
		}
		d->selecting = false;
		applySelection();
	}
	return true;
}

QList<int> SubtitleRenderer::autoselection(const Mrl &mrl, const QList<Loaded> &loaded) {
	QList<int> selected;
	if (loaded.isEmpty() || !mrl.isLocalFile())
		return selected;
	const Pref &p = Pref::get();
	QSet<QString> langSet;
	const QString base = QFileInfo(mrl.toLocalFile()).completeBaseName();
	for (int i=0; i<loaded.size(); ++i) {
		bool select = false;
		if (p.sub_autoselect == Enum::SubtitleAutoselect::Matched) {
			select = QFileInfo(loaded[i].m_comp.fileName()).completeBaseName() == base;
		} else if (p.sub_autoselect == Enum::SubtitleAutoselect::All) {
			select = true;
		} else if (p.sub_autoselect == Enum::SubtitleAutoselect::EachLanguage) {
			const QString lang = loaded[i].m_comp.language().id();
			if ((select = (!langSet.contains(lang))))
				langSet.insert(lang);
		}
		if (select)
			selected.append(i);
	}
	if (p.sub_autoselect == Enum::SubtitleAutoselect::Matched
			&& !selected.isEmpty() && !p.sub_ext.isEmpty()) {
		for (int i=0; i<selected.size(); ++i) {
			const QString fileName = loaded[selected[i]].m_comp.fileName();
			const QString suffix = QFileInfo(fileName).suffix().toLower();
			if (p.sub_ext == suffix) {
				const int idx = selected[i];
				selected.clear();
				selected.append(idx);
				break;
			}
		}
	}
	return selected;
}

int SubtitleRenderer::autoload(const Mrl &mrl, bool autoselect) {
	unload();
	const Pref &pref = Pref::get();
	if (pref.sub_autoload == Enum::SubtitleAutoload::None)
		return 0;
	const QStringList filter = Info::subtitleNameFilter();
	const QFileInfo fileInfo(mrl.toLocalFile());
	const QFileInfoList all = fileInfo.dir().entryInfoList(filter, QDir::Files, QDir::Name);
	const QString base = fileInfo.completeBaseName();
	for (int i=0; i<all.size(); ++i) {
		if (pref.sub_autoload != Enum::SubtitleAutoload::Folder) {
			if (pref.sub_autoload == Enum::SubtitleAutoload::Matched) {
				if (base != all[i].completeBaseName())
					continue;
			} else if (!all[i].fileName().contains(base))
				continue;
		}
		Subtitle sub;
		if (sub.load(all[i].absoluteFilePath(), pref.sub_enc)) {
			for (int i=0; i<sub.size(); ++i)
				d->loaded.push_back(Loaded(sub[i]));
		}
	}
	if (autoselect) {
		const QList<int> selected = autoselection(mrl, d->loaded);
		d->selecting = true;
		for (int i=0; i<selected.size(); ++i) {
			select(selected[i], true);
		}
		d->selecting = false;
		applySelection();
	}
	return d->loaded.size();
}

int SubtitleRenderer::start(int time) const {
	int s = -1;
	RenderList::const_iterator it = d->order.begin();
	for (; it != d->order.end(); ++it) {
		const Comp *comp = (*it)->comp;
		const CompIt it = comp->start(time - d->delay, d->fps);
		if (it != comp->end())
			s = qMax(s, comp->isBasedOnFrame() ? Subtitle::msec(it.key(), d->fps) : it.key());
	}
	return s;
}

int SubtitleRenderer::end(int time) const {
	int e = -1;
	RenderList::const_iterator it = d->order.begin();
	for (; it != d->order.end(); ++it) {
		const Comp *comp = (*it)->comp;
		const CompIt it = comp->end(time - d->delay, d->fps);
		if (it != comp->end()) {
			const int t = comp->isBasedOnFrame() ? Subtitle::msec(it.key(), d->fps) : it.key();
			e = e == -1 ? t : qMin(e, t);
		}
	}
	return e;
}

void SubtitleRenderer::setDelay(int delay) {
	if (d->delay != delay) {
		d->delay = delay;
		render(d->ms);
	}
}

bool SubtitleRenderer::hasSubtitle() const {
	return !d->empty;
}

void SubtitleRenderer::setPos(double pos) {
	if (!qFuzzyCompare(pos, d->pos)) {
		d->pos = qBound(0.0, pos, 1.0);
		if (d->top)
			d->osd->setMargin(d->pos, 0, 0, 0);
		else
			d->osd->setMargin(0, 1.0 - d->pos, 0, 0);
	}
}

double SubtitleRenderer::pos() const {
	return d->pos;
}

void SubtitleRenderer::setTopAlignment(bool top) {
	if (d->top != top) {
		d->top = top;
		Qt::Alignment alignment = Qt::AlignHCenter;
		if (d->top)
			alignment |= Qt::AlignTop;
		else
			alignment |= Qt::AlignBottom;
		d->osd->setAlignment(alignment);
		setPos(1.0 - d->pos);
	}
}

bool SubtitleRenderer::isTopAligned() const {
	return d->top;
}

int SubtitleRenderer::current() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	for (; o != d->order.end(); ++o) {
		Render &render = **o;
		if (render.prev == render.comp->end() || !render.prev->text.hasWords())
			continue;
		if (time < 0)
			time = render.prev.key();
		else if (render.prev.key() > time)
			time = render.prev.key();
	}
	return time;
}

int SubtitleRenderer::previous() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	QList<CompIt> its;
	its.reserve(d->order.size());
	for (; o != d->order.end(); ++o) {
		Render &r = **o;
		if (r.prev == r.comp->end())
			continue;
		CompIt it = r.prev;
		while (it != r.comp->begin()) {
			if ((--it)->text.hasWords()) {
				if (time < 0)
					time = it.key();
				else if (it.key() > time)
					time = it.key();
				break;
			}
		}
	}
	return time;
}

int SubtitleRenderer::next() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	QList<CompIt> its;
	its.reserve(d->order.size());
	for (; o != d->order.end(); ++o) {
		Render &r = **o;
		if (r.prev == r.comp->end())
			continue;
		CompIt it = r.prev;
		while (++it != r.comp->end()) {
			if (it->text.hasWords()) {
				if (time < 0)
					time = it.key();
				else if (it.key() < time)
					time = it.key();
				break;
			}
		}
	}
	return time;
}
