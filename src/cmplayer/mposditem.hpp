#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"

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

class MpOsdItem : public TextureRendererItem {
public:
	MpOsdItem(QQuickItem *parent = nullptr);
	~MpOsdItem();
	void draw(sub_bitmaps *imgs);
	void setFrameSize(const QSize &size);
	void present();
	void draw(QImage &frame);
private:
	static const int ShowEvent = QEvent::User + 1;
	static const int HideEvent = QEvent::User + 2;
	void customEvent(QEvent *event);
	bool blending() const override {return true;}
	void prepare();
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	void beforeUpdate();
	void initializeTextures();
	const char *fragmentShader() const;
	struct Data;
	Data *d;
};

#endif // MPOSDITEM_HPP
