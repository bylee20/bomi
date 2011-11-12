#ifndef OSDRENDERER_HPP
#define OSDRENDERER_HPP

#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtGui/QPixmap>

class RichString;	class OsdStyle;
class QPainter;

class OsdRenderer : public QObject {
	Q_OBJECT
public:
	static int cachedSize(double size);
	static int cachedSize(int size) {return cachedSize(double(size));}
	static QSize cachedSize(const QSize &size) {
		return QSize(cachedSize(size.width()), cachedSize(size.height()));
	}
	static QSize cachedSize(const QSizeF &size) {
		return QSize(cachedSize(size.width()), cachedSize(size.height()));
	}

	OsdRenderer(): m_letterbox(true) {}
	~OsdRenderer() {}
	virtual void render(QPainter *painter, const QPointF &pos) = 0;
	virtual QPointF posHint() const = 0;
	virtual QSizeF size() const = 0;
	virtual const OsdStyle &style() const = 0;
	void setBackgroundSize(const QSize &screen, const QSizeF &video);
	double width() const {return size().width();}
	double height() const {return size().height();}
	void setLetterboxHint(bool letterbox);
	bool letterboxHint() const {return m_letterbox;}
	QSizeF backgroundSize() const {return m_letterbox ? m_screen : m_video;}
	virtual bool hasCached() const {return false;}
public slots:
	virtual void setStyle(const OsdStyle &style) = 0;
signals:
	void sizeChanged(const QSizeF &size);
	void styleChanged(const OsdStyle &style);
	void needToRerender();
protected:
	virtual void updateBackgroundSize(const QSizeF &size) = 0;
private:
	bool m_letterbox;
	QSizeF m_video;
	QSize m_screen;
};

#endif // OSDRENDERER_HPP
