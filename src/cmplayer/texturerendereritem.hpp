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
	double devicePixelRatio() const { return m_win ? m_win->devicePixelRatio() : 1.0; }
	void setDithering(Dithering dithering) { if (_Change(m_newDithering, dithering)) rerender(); }
	Dithering dithering() const { return m_newDithering; }
	int depth() const { return 8; }
protected slots:
	virtual void initializeGL() { m_lutInt1.generate(); m_lutInt2.generate(); m_ditheringTex.generate(); }
	virtual void finalizeGL() { m_lutInt1.delete_(); m_lutInt2.delete_(); m_ditheringTex.delete_(); }
protected:
	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
	void setGeometryDirty() { m_dirtyGeomerty = true; }
	void setRenderTarget(const OpenGLTexture &texture) { m_texture = texture; }
	virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	virtual void prepare(QSGGeometryNode *node) = 0;
	virtual void getCoords(QRectF &vertices, QRectF &/*texCoords*/) { vertices = boundingRect(); }
	virtual QSGMaterialType *shaderId() const { return &m_types[m_intCategory][m_dithering > 0]; }
	virtual TextureRendererShader *createShader() const;
private slots:
	void tryInitGL() { if (!m_init && QOpenGLContext::currentContext()) { initializeGL(); m_init = true; } }
private:
	const OpenGLTexture &texture() const { return m_texture; }
	const OpenGLTexture &ditheringTexture() const { return m_ditheringTex; }
	const InterpolatorLutTexture &lutInterpolatorTexture1() const { return m_lutInt1; }
	const InterpolatorLutTexture &lutInterpolatorTexture2() const { return m_lutInt2; }
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final override;
	friend class TextureRendererShader;
	struct Material;	struct Node;	struct Data; Data *d;
	bool m_dirtyGeomerty = true, m_init = false;
	InterpolatorType m_interpolator = InterpolatorType::Bilinear;
	InterpolatorType m_newInt = InterpolatorType::Bilinear;
	mutable QSGMaterialType m_types[4][2];
	int m_intCategory = 0;
	InterpolatorLutTexture m_lutInt1, m_lutInt2;
	OpenGLTexture m_texture, m_ditheringTex;
	QQuickWindow *m_win = nullptr;
	Dithering m_dithering = Dithering::None, m_newDithering = Dithering::None;
};

class TextureRendererShader : public QSGMaterialShader {
public:
	TextureRendererShader(const TextureRendererItem *item, int interpolator = 0, bool dithering = false);
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
	int m_interpolator = 0;
	bool m_dithering = false;
	int loc_tex = -1, loc_lut_int1 = -1, loc_lut_int1_mul = -1, loc_lut_int2 = -1, loc_lut_int2_mul = -1, loc_vMatrix = -1, loc_dxy = -1;
	int loc_dithering = -1, loc_dithering_quantization = -1, loc_dithering_center = -1, loc_dithering_size = -1;
	QByteArray m_fragCode, m_vertexCode;
};

#endif // TEXTURERENDERERITEM_HPP
