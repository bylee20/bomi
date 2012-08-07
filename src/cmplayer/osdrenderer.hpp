#ifndef OSDRENDERER_HPP
#define OSDRENDERER_HPP

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include "osdstyle.hpp"

class QPainter;

class OsdRenderer : public QObject {
	Q_OBJECT
public:
	OsdRenderer();
	~OsdRenderer();
	virtual void prepareToRender(const QPointF &/*pos*/) {}
	void render(QPainter *painter, const QPointF &pos) {
		const int count = layers();
		for (int i=count-1; i>=0; --i)
			render(painter, pos, i);
	}

	virtual void render(QPainter *painter, const QPointF &pos, int layer) = 0;
	virtual QPointF posHint() const = 0;
	virtual QSizeF size() const = 0;
	const OsdStyle &style() const {return m_style;}
	bool setArea(const QSize &screen, const QSizeF &frame);
	double width() const {return size().width();}
	double height() const {return size().height();}
	void setLetterboxHint(bool letterbox);
	bool letterboxHint() const {return m_letterbox;}
	QSizeF renderableSize() const {return m_letterbox ? m_screen : m_frame;}
	virtual int layers() const {return 1;}
	double scale() const;
public slots:
	void setStyle(const OsdStyle &style);
	virtual void clear() = 0;
signals:
	void sizeChanged(const QSizeF &size);
	void styleChanged(const OsdStyle &style);
	void needToRerender();
protected:
	virtual bool updateRenderableSize(const QSizeF &renderable) = 0;
	virtual void updateStyle(const OsdStyle &/*style*/) {}
private:
	bool m_letterbox;
	QSizeF m_frame;
	QSize m_screen;
	OsdStyle m_style;
};

#endif // OSDRENDERER_HPP
