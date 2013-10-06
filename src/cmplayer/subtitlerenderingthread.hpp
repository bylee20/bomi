#ifndef SUBTITLERENDERINGTHREAD_HPP
#define SUBTITLERENDERINGTHREAD_HPP

#include "stdafx.hpp"
#include "subtitle.hpp"
#include "dataevent.hpp"

class SubtitleComponentModel;	class SubtitleDrawer;

class SubtitleRenderingThread : public QThread {
	Q_OBJECT
public:
	typedef SubtitleComponent::const_iterator CompIt;
	typedef QList<const SubtitleComponent*> RenderTarget;
	typedef QMap<int, QList<CompIt> >::const_iterator TimeIterator;
	struct Picture {
		Picture(const QList<CompIt> &its, const RenderTarget &target): its(its) {
			Q_ASSERT(its.size() == target.size());
			for (int i=0; i<its.size(); ++i) {
				if (its[i] != target[i]->end())
					text += its[i].value();
			}
		}
		Picture(const RenderTarget &target) {
			for (int i=0; i<target.size(); ++i)
				its.append(target[i]->end());
		}
		RichTextDocument text;
		QImage image;
		QSize size;
		QPointF shadow;
		QList<CompIt> its;
	};
public:
	enum EventType {SetDrawer = QEvent::User + 1, Tick, SetArea, Reset, Prepared};
	SubtitleRenderingThread(QObject *parent);
	~SubtitleRenderingThread();
	void reset(const RenderTarget &list, bool sync = false);
	void clear(bool sync = false) { reset(RenderTarget(), sync); }
	void setDrawer(const SubtitleDrawer &drawer);
	void render(int time, double fps) { postData(this, Tick, time, fps, true); }
	void rerender(int time, double fps) { postData(this, Tick, time, fps, false); }
	void setArea(const QRectF &rect, double dpr) { if (m_area != rect || m_dpr != dpr) postData(this, SetArea, rect, dpr); }
private:
	void newImage(const Picture &pic);
	Picture draw(const QList<CompIt> &its);
	QList<CompIt> iterators(int time) const;
	void rebuild();
	bool event(QEvent *event);
	void updateCache();
	void update();
	struct Data; Data *d;
	const RenderTarget &target() const { return m_target; }
	QRectF m_area; double m_dpr = 1.0, m_fps = 30.0;
	RenderTarget m_target;
};

#endif // SUBTITLERENDERINGTHREAD_HPP
