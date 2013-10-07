#include "subtitlerenderingthread.hpp"

static inline bool operator < (const SubCompItMapIt &lhs, const SubCompItMapIt &rhs) {
	return lhs.key() < rhs.key();
}

SubCompThread::SubCompThread(QMutex *mutex, QWaitCondition *wait, const SubComp *comp, QObject *renderer)
: QThread(), m_comp(comp) {
	m_renderer = renderer;
	m_it = m_its.end();
	m_mutex = mutex;
	m_wait = wait;
}

SubCompThread::~SubCompThread() {
	finish();
}

void SubCompThread::finish() {
	m_quit = true;
	m_wait->wakeAll();
	if (!wait(5000))
		terminate();
}

void SubCompThread::run() {
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

	auto rebuild = [this] () {
		m_pool.clear();
		m_its.clear();
		m_it = m_its.end();
		for (auto it = m_comp->begin(); it != m_comp->end(); ++it)
			m_its.insert(m_comp->toTime(it.key(), m_key.fps), it);
	};

	while (!m_quit) {
		QMutexLocker locker(m_mutex);
		m_wait->wait(m_mutex);
		if (m_quit)
			break;
		int changed = m_changed;
		m_changed = 0;
		m_key = m_newKey;
		if (changed & Option)
			m_option = m_newOption;
		locker.unlock();
		if (m_quit)
			break;
		if (changed & Rebuild)
			rebuild();
		if (changed & Option)
			m_pool.clear();
		if (m_key.time < 0 || m_key.fps < 0.0)
			continue;
		if (m_quit)
			break;
		if (!m_its.isEmpty())
			draw(m_key.time, changed & ForceUpdate);
	}
}

SubCompPicture SubCompThread::picture(SubCompIt it) {
	SubCompPicture pic(m_comp, it);
	m_option.drawer.draw(pic.text(), pic.m_image, pic.m_size, pic.m_shadow, m_option.area, m_option.dpr);
	return pic;
}

void SubCompThread::update() {
	auto post = [this] (const SubCompPicture &pic) { postData(m_renderer, Prepared, pic); };
	if (m_it != m_its.end()) {
		auto cache = m_pool.find(m_it);
		if (cache == m_pool.end())
			cache = m_pool.insert(m_it, picture(*m_it));
		post(*cache);
	} else
		post(m_comp);
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
			it = m_pool.insert(key, picture(*key));
	}
}
