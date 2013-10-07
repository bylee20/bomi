#ifndef SUBTITLERENDERINGTHREAD_HPP
#define SUBTITLERENDERINGTHREAD_HPP

#include "stdafx.hpp"
#include "subtitle.hpp"
#include "dataevent.hpp"
#include "subtitledrawer.hpp"

class SubtitleComponentModel;	class SubtitleDrawer;
using SubCompIt = SubComp::const_iterator;
using SubCompItMap = QMap<int, SubCompIt>;
using SubCompItMapIt = SubCompItMap::const_iterator;

class SubCompPicture {
public:
	SubCompPicture(const SubComp *comp, SubCompIt it)
	: m_comp(comp), m_it(it) { if (m_it != comp->end()) m_text = *m_it; }
	SubCompPicture(const SubComp *comp): m_comp(comp) { if (comp) m_it = m_comp->end(); }
	SubCompIt iterator() const { return m_it; }
	const QSize &size() const { return m_size; }
	const QImage &image() const { return m_image; }
	const QPointF &shadowOffset() const { return m_shadow; }
	const RichTextDocument &text() const { return m_text; }
	const SubComp *component() const { return m_comp; }
private:
	friend class SubCompThread;
	const SubComp *m_comp = nullptr;
	SubCompIt m_it; RichTextDocument m_text;
	QImage m_image; QSize m_size; QPointF m_shadow;
};

class SubCompThread : public QThread {
	Q_OBJECT
public:
	enum {Option = 1, Rebuild = 4, FPS = 8, Time = 16, ForceUpdate = Option | FPS};
	enum EventType {SetDrawer = QEvent::User + 1, SetArea, Reset, Prepared};
	SubCompThread(QMutex *mutex, QWaitCondition *wait, const SubComp *comp, QObject *renderer);
	~SubCompThread();
	void render(int time, double fps) {
		m_newKey.time = time;
		if (_Change(m_newKey.fps, fps))
			m_changed |= Rebuild;
	}
	void rerender(int time, double fps) {
		render(time, fps);
		m_changed |= FPS;
	}
	void setArea(const QRectF &rect, double dpr) {
		if (_Change(m_newOption.area, rect))
			m_changed |= Option;
		if (_Change(m_newOption.dpr, dpr))
			m_changed |= Option;
	}
	void setDrawer(const SubtitleDrawer &drawer) {
		m_newOption.drawer = drawer;
		m_changed |= Option;
	}
	void finish();
private:
	void run();
	void postPicture(const SubCompPicture &pic) { postData(m_renderer, Prepared, pic); }
	SubCompPicture picture(SubCompIt it);
	SubCompIt iterator(int time) const { return m_comp->start(time, m_key.fps); }
	void updateCache();
	void update();
	struct DrawOption { QRectF area; double dpr = 1.0; SubtitleDrawer drawer; };
	struct DrawKey { int time = 0; double fps = -1.0; };
	DrawOption m_newOption, m_option;
	DrawKey m_newKey, m_key;
	const SubComp *m_comp = nullptr;
	int m_changed = 0;
	SubCompItMap m_its; SubCompItMapIt m_it;
	QMap<SubCompItMapIt, SubCompPicture> m_pool;
	QObject *m_renderer = nullptr;
	bool m_quit = false;
	QMutex *m_mutex;
	QWaitCondition *m_wait;
};

struct SubCompObject {
	SubCompThread *thread = nullptr;
	const SubComp *comp = nullptr;
	int order = -1;
	SubCompPicture picture{nullptr};
	SubtitleComponentModel *model = nullptr;
};

#endif // SUBTITLERENDERINGTHREAD_HPP
