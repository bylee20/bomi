#include "subtitlerenderingthread.hpp"
#include "subtitledrawer.hpp"

static inline bool operator < (const SubtitleRenderingThread::TimeIterator &lhs, const SubtitleRenderingThread::TimeIterator &rhs) {
	return lhs.key() < rhs.key();
}

struct SubtitleRenderingThread::Data {
	SubtitleDrawer drawer;
	QMap<int, QList<CompIt> > its;
	QMap<int, QList<CompIt> >::const_iterator current;
	QMap<TimeIterator, Picture> pool;
};

SubtitleRenderingThread::SubtitleRenderingThread(const RenderTarget &target, QObject *parent)
: QThread(parent), d(new Data), m_target(target) {
	d->current = d->its.end();
}

SubtitleRenderingThread::~SubtitleRenderingThread() { quit(); delete d; }

void SubtitleRenderingThread::setDrawer(const SubtitleDrawer &drawer) { post(this, SetDrawer, drawer); }

void SubtitleRenderingThread::newImage(const Picture &pic) { post(parent(), Prepared, this, pic); }

SubtitleRenderingThread::Picture SubtitleRenderingThread::draw(const QList<CompIt> &its) {
	Picture pic(its, m_target);
	d->drawer.setText(pic.text);
	d->drawer.draw(pic.image, pic.size, pic.shadow, m_area, m_dpr);
	return pic;
}

QList<SubtitleRenderingThread::CompIt> SubtitleRenderingThread::iterators(int time) const {
	QList<CompIt> its;
	for (auto *comp : m_target)
		its.append(comp->start(time, m_fps));
	return its;
}

void SubtitleRenderingThread::rebuild() {
	d->pool.clear();
	d->its.clear();
	for (auto comp : m_target) {
		for (auto it = comp->begin(); it != comp->end(); ++it) {
			const int time = comp->toTime(it.key(), m_fps);
			d->its.insert(time, QList<CompIt>());
		}
	}
	for (auto it = d->its.begin(); it != d->its.end(); ++it)
		*it = iterators(it.key());
	d->current = d->its.end();
}

void SubtitleRenderingThread::update() {
	if (d->current != d->its.end()) {
		auto cache = d->pool.find(d->current);
		if (cache == d->pool.end())
			cache = d->pool.insert(d->current, draw(*d->current));
		newImage(*cache);
	} else {
		newImage(Picture(m_target));
	}
}

bool SubtitleRenderingThread::event(QEvent *event) {
	const int type = event->type();
	switch (type) {
	case SetDrawer:
		get(event, d->drawer);
		d->pool.clear();
		update();
		return true;
	case SetArea:
		get(event, m_area, m_dpr);
		d->pool.clear();
		update();
		return true;
	case Reset:
		return true;
	case Tick: {
		int time; double fps; bool check = true;
		get(event, time, fps, check);
		if (time < 0 || fps < 0.0)
			return true;
		if (_Change(m_fps, fps))
			rebuild();
		if (d->its.isEmpty())
			return true;
		auto it = --d->its.upperBound(time);
		if (!check || d->current != it) {
			if (d->current == d->its.end() || ++d->current != it) {
				d->pool.clear();
				d->current = it;
				update();
			} else {
				update();
				updateCache();
			}
		}
		return true;
	} default:
		return QThread::event(event);
	}
}

void SubtitleRenderingThread::updateCache() {
	if (d->current == d->its.end())
		return;
	auto it = d->pool.begin();
	while (it != d->pool.end() && it.key() < d->current)
		it = d->pool.erase(it);
	Q_ASSERT(it.key() == d->current);
	auto key = d->current;
	const int size = qMin(4, d->pool.size()+1);
	for (int i=0; i<size; ++i) {
		++it; ++key;
		if (key == d->its.end())
			break;
		if (it == d->pool.end())
			it = d->pool.insert(key, draw(*key));
	}
}
