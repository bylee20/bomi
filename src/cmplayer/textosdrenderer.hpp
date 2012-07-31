#ifndef TEXTOSDRENDERER_HPP
#define TEXTOSDRENDERER_HPP

#include "osdrenderer.hpp"
#include "subtitle.hpp"

class TextOsdRenderer : public OsdRenderer {
	Q_OBJECT
public:
	TextOsdRenderer(Qt::Alignment align = Qt::AlignTop | Qt::AlignHCenter);
	~TextOsdRenderer();
	void show(const QString &text, int last = 2500);
	void show(const RichTextDocument &doc, int last = 2500);
	void prepareToRender(const QPointF &);
	void render(QPainter *painter, const QPointF &pos, int layer);
	QPointF posHint() const;
	QSizeF size() const;
	void setAlignment(Qt::Alignment alignment);
	Qt::Alignment alignment() const;
	QString text() const;
	void setMargin(double top, double bottom, double right, double left);
	bool isText() const {return true;}
	int layers() const {return 2;}
	double scale() const;
public slots:
	void clear();
private:
	bool updateRenderableSize(const QSizeF &renderable);
	void updateStyle(const OsdStyle &);
	void updateFont();
	struct Data;
	Data *d;
};

#endif // TEXTOSDRENDERER_HPP
