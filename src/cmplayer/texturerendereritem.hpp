#ifndef TEXTURERENDERERITEM_HPP
#define TEXTURERENDERERITEM_HPP

#include "stdafx.hpp"

class TextureRendererItem : public QQuickItem {
	Q_OBJECT
public:
	TextureRendererItem(int textures, QQuickItem *parent = 0);
	~TextureRendererItem();
protected:
	typedef QSGGeometry::TexturedPoint2D TexturedPoint2D;
	typedef QSGMaterialShader::RenderState RenderState;
	virtual char const *const *attributeNames() const;
	virtual const char *vertexShader() const;
	virtual const char *fragmentShader() const = 0;
	virtual void link(QOpenGLShaderProgram *program);
	virtual void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void setGeometryDirty(bool dirty = true);
	virtual QRectF vertexRect() const {return boundingRect();}
	virtual QRectF textureRect() const {return QRectF(0.0, 0.0, 1.0, 1.0);}
	virtual bool beforeUpdate() {return false;}
	virtual void updateTexturedPoint2D(TexturedPoint2D *tp) {
		const auto vtx = vertexRect();
		const auto txt = textureRect();
		set(tp, vtx.topLeft(), txt.topLeft());
		set(++tp, vtx.bottomLeft(), txt.bottomLeft());
		set(++tp, vtx.topRight(), txt.topRight());
		set(++tp, vtx.bottomRight(), txt.bottomRight());
	}
	void set(TexturedPoint2D *tp, const QPointF &vtx, const QPointF &txt) {
		tp->set(vtx.x(), vtx.y(), txt.x(), txt.y());
	}
	GLuint texture(int i) const {return m_textures[i];}
	virtual bool blending() const {return false;}
private:
	struct Shader;	struct Material;	struct Node;
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final;
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	GLuint *m_textures = nullptr;
	struct Data;
	Data *d;
};

#endif // TEXTURERENDERERITEM_HPP
