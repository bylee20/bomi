#ifndef TEXTURERENDERERITEM_HPP
#define TEXTURERENDERERITEM_HPP

#include "stdafx.hpp"
#include "geometryitem.hpp"
#include "enums.hpp"
#include "openglcompat.hpp"

class TextureRendererShader;

enum class InterpolatorType;
class OpenGLTexture;

class TextureRendererItem : public GeometryItem {
	Q_OBJECT
public:
	TextureRendererItem(QQuickItem *parent = 0);
	~TextureRendererItem();
	virtual bool isOpaque() const { return false; }
	InterpolatorType interpolator() const { return m_newInt; }
	void setInterpolator(InterpolatorType type) { if (_Change(m_newInt, type)) rerender(); }
	virtual void rerender() { update(); }
	bool isGeometryDirty() const { return m_dirtyGeomerty; }
	virtual int quads() const { return 1; }
protected slots:
	virtual void initializeGL() { m_lutInt.generate(); }
	virtual void finalizeGL() { m_lutInt.delete_(); }
protected:
	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
	void setGeometryDirty() { m_dirtyGeomerty = true; }
	void setRenderTarget(const OpenGLTexture &texture) { m_texture = texture; }
	virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	virtual void prepare(QSGGeometryNode *node) = 0;
	virtual void getCoords(QRectF &vertices, QRectF &/*texCoords*/) { vertices = boundingRect(); }
	virtual QSGMaterialType *shaderId() const { return &m_types[m_interpolator > 0]; }
	virtual TextureRendererShader *createShader() const;
private slots:
	void tryInitGL() { if (!m_init && QOpenGLContext::currentContext()) { initializeGL(); m_init = true; } }
private:
	const OpenGLTexture &texture() const { return m_texture; }
	const OpenGLTexture &lutInterpolatorTexture() const { return m_lutInt; }
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final override;
	friend class TextureRendererShader;
	struct Material;	struct Node;	struct Data; Data *d;
	bool m_dirtyGeomerty = true, m_init = false;
	InterpolatorType m_interpolator = InterpolatorType::Bilinear;
	InterpolatorType m_newInt = InterpolatorType::Bilinear;
	mutable QSGMaterialType m_types[2];
	OpenGLTexture m_lutInt, m_texture;
};

class TextureRendererShader : public QSGMaterialShader {
public:
	TextureRendererShader(const TextureRendererItem *item, bool interpolator = false);
	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
	const char *fragmentShader() const override { return m_fragCode.constData(); }
	const char *vertexShader() const override { return m_vertexCode.constData(); }
	const char *const *attributeNames() const override;
	virtual void link(QOpenGLShaderProgram *prog);
	virtual void bind(QOpenGLShaderProgram *prog);
	const TextureRendererItem *item() const { return m_item; }
private:
	void updateState(const RenderState &state, QSGMaterial */*new_*/, QSGMaterial */*old*/) final override;
	void initialize() final override;
	const TextureRendererItem *m_item = nullptr;
	bool m_interpolator = false;
	int loc_tex = -1, loc_lut_interpolator = -1, loc_vMatrix = -1, loc_dxy = -1;
	QByteArray m_fragCode, m_vertexCode;
};

#endif // TEXTURERENDERERITEM_HPP
