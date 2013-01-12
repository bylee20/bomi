#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"

class MpOsdItem : public TextureRendererItem {
public:
	MpOsdItem(QQuickItem *parent = nullptr);
	~MpOsdItem();
	void draw(int x, int y, int w, int h, uchar *src, uchar *srca, int stride);
	void setFrameSize(const QSize &size);
	void setFrameChanged();
	QMutex &mutex() const;
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
