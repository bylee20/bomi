#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"

enum EventType {EnqueueFrame = QEvent::User + 1, RenderNextFrame, EmptyQueue, UpdateDeint, Show, Hide };

class LetterboxItem : public QQuickItem {
public:
    LetterboxItem(QQuickItem *parent = 0);
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
    bool set(const QRectF &outer, const QRectF &inner);
    const QRectF &screen() {return m_screen;}
private:
    QRectF m_outer, m_inner, m_screen;
    bool m_rectChanged;
};

struct sub_bitmaps;

class MpOsdItem : public QQuickItem {
public:
	MpOsdItem(QQuickItem *parent = nullptr);
	~MpOsdItem();
	void drawOn(sub_bitmaps *imgs);
	void setFrameSize(const QSize &size);
	QSize frameSize() const;
	void present();
	void drawOn(QImage &frame);
private:
	friend class MpOsdItemShader;
	void updateState(QOpenGLShaderProgram *program);
	void customEvent(QEvent *event);
	void geometryChanged(const QRectF &newOne, const QRectF &old);
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data);
	struct Data;
	Data *d;
};

#endif // MPOSDITEM_HPP
