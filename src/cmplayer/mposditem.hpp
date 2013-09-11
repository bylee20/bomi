#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"

enum EventType {EnqueueFrame = QEvent::User + 1, RenderNextFrame, EmptyQueue, UpdateDeint, Show, Hide };

struct sub_bitmaps;



class MpOsdItem : public QQuickItem {
public:
	MpOsdItem(QQuickItem *parent = nullptr);
	~MpOsdItem();
	void drawOn(sub_bitmaps *imgs);
	void present();
	void drawOn(QImage &frame);
	QSize targetSize() const;
	void setRenderSize(const QSize &size);
	void forceUpdateTargetSize();
private:
	void customEvent(QEvent *event);
	void geometryChanged(const QRectF &newOne, const QRectF &old);
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data);
	struct Data;
	Data *d;
};

#endif // MPOSDITEM_HPP
