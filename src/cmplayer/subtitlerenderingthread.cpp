#include "subtitlerenderingthread.hpp"

static inline bool operator < (const SubCompItMapIt &lhs, const SubCompItMapIt &rhs) {
	return lhs.key() < rhs.key();
}

SubCompThread::SubCompThread(const SubComp *comp, QObject *renderer)
: QThread(), m_comp(comp) {
	m_thread = thread();
	moveToThread(this);
	m_renderer = renderer;
	m_it = m_its.end();
}

SubCompThread::~SubCompThread() {
	quit();
	if (!wait(5000))
		terminate();
}

void SubCompThread::run() {
	exec();
	moveToThread(m_thread);
}

void SubCompThread::rebuild() {
	m_pool.clear();
	m_drawer.clear();
	m_its.clear();
	m_it = m_its.end();
	for (auto it = m_comp->begin(); it != m_comp->end(); ++it)
		m_its.insert(m_comp->toTime(it.key(), m_fps), it);
}

void SubCompThread::update() {
	if (m_it != m_its.end()) {
		auto cache = m_pool.find(m_it);
		if (cache == m_pool.end())
			cache = m_pool.insert(m_it, draw(*m_it));
		postPicture(*cache);
	} else
		postPicture(m_comp);
}

bool SubCompThread::event(QEvent *event) {
	auto draw = [this] (int time, bool force) {
		auto it = --m_its.upperBound(time);
		if (force || m_it != it) {
			if (m_it == m_its.end() || ++m_it != it) {
				m_pool.clear();
				m_it = it;
				update();
			} else {
				update();
				updateCache();
			}
		}
	};

	const int type = event->type();
	switch (type) {
	case SetDrawer:
		getAllData(event, m_drawer);
		m_pool.clear();
		update();
		return true;
	case SetArea:
		getAllData(event, m_area, m_dpr);
		m_pool.clear();
		update();
		return true;
	case Tick: {
		int time; double fps; bool force = false;
		getAllData(event, time, fps, force);
		if (time < 0 || fps < 0.0)
			return true;
		if (_Change(m_fps, fps))
			rebuild();
		if (!m_its.isEmpty())
			draw(time, force);
		return true;
	} default:
		return QThread::event(event);
	}
}

void SubCompThread::updateCache() {
	if (m_it == m_its.end())
		return;
	auto it = m_pool.begin();
	while (it != m_pool.end() && it.key() < m_it)
		it = m_pool.erase(it);
	Q_ASSERT(it.key() == m_it);
	auto key = m_it;
	const int size = qMin(2, m_pool.size()+1);
	for (int i=0; i<size; ++i) {
		++it; ++key;
		if (key == m_its.end())
			break;
		if (it == m_pool.end())
			it = m_pool.insert(key, draw(*key));
	}
}
