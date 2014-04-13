#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"
#include "subtitle.hpp"

class RichTextDocument;		class SubCompModel;
struct SubtitleStyle;		class SubtitleDrawer;

class SubtitleRendererItem : public HQTextureRendererItem  {
	Q_OBJECT
public:
	SubtitleRendererItem(QQuickItem *parent = nullptr);
	~SubtitleRendererItem();
	int previous() const;
	int next() const;
	int current() const;
	int start(int pos) const;
	int finish(int pos) const;
	int delay() const;
	double fps() const;
	double pos() const;
	bool isTopAligned() const;
	QVector<SubCompModel*> models() const;
	QList<const SubComp *> components() const;
	void setComponents(const QList<SubComp> &components);
	int componentsCount() const;
	void setPriority(const QStringList &priority);
	void setPos(double pos);
	bool isHidden() const;
	void setDelay(int delay);
	bool load(const Subtitle &subtitle, bool select);
	void unload();
	void select(int idx);
	void deselect(int idx = -1);
	const SubtitleStyle &style() const;
	void setStyle(const SubtitleStyle &style);
	const RichTextDocument &text() const;
	bool isOpaque() const override {return false;}
	QImage draw(const QRectF &rect, QRectF *put = nullptr) const;
public slots:
	void setHidden(bool hidden);
	void render(int ms);
	void setTopAligned(bool top);
	void setFPS(double fps);
signals:
	void modelsChanged(const QVector<SubCompModel*> &models);
private:
	void initializeGL();
	void finalizeGL();
	void rerender();
	void customEvent(QEvent *event) override;
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
	void prepare(QSGGeometryNode *node) override;
	void getCoords(QRectF &vertices, QRectF &) override;
	TextureRendererShader *createShader() const override;
	struct Data; Data *d;
	friend class SubtitleRendererShader;
};

#endif // SUBTITLERENDERERITEM_HPP
