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
	enum EventType {SetDrawer = QEvent::User + 1, Tick, SetArea, Reset, Prepared};
	SubCompThread(const SubComp *comp, QObject *renderer);
	~SubCompThread();
	void setDrawer(const SubtitleDrawer &drawer) { postData(this, SetDrawer, drawer); }
	void render(int time, double fps) { postData(this, Tick, time, fps, false); }
	void rerender(int time, double fps) { postData(this, Tick, time, fps, true); }
	void setArea(const QRectF &rect, double dpr) {
		if (m_area != rect || m_dpr != dpr) postData(this, SetArea, rect, dpr);
	}
private:
	void run();
	void postPicture(const SubCompPicture &pic) { postData(m_renderer, Prepared, pic); }
	SubCompPicture draw(SubCompIt it) {
		SubCompPicture pic(m_comp, it);
		m_drawer.setText(pic.text());
		m_drawer.draw(pic.m_image, pic.m_size, pic.m_shadow, m_area, m_dpr);
		return pic;
	}
	SubCompIt iterator(int time) const { return m_comp->start(time, m_fps); }
	void rebuild();
	bool event(QEvent *event);
	void updateCache();
	void update();
	QRectF m_area; double m_dpr = 1.0, m_fps = 30.0;
	const SubComp *m_comp = nullptr;
	SubtitleDrawer m_drawer;
	SubCompItMap m_its; SubCompItMapIt m_it;
	QMap<SubCompItMapIt, SubCompPicture> m_pool;
	QObject *m_renderer = nullptr;
	bool m_reset = false;
	QThread *m_thread = nullptr;
};

struct SubCompObject {
	SubCompThread *thread = nullptr;
	const SubComp *comp = nullptr;
	int order = -1;
	SubCompPicture picture{nullptr};
	SubtitleComponentModel *model = nullptr;
};

#endif // SUBTITLERENDERINGTHREAD_HPP
