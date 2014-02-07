#ifndef TEXTURERENDERERITEM_HPP
#define TEXTURERENDERERITEM_HPP

#include "stdafx.hpp"
#include "geometryitem.hpp"
#include "enums.hpp"
#include "openglcompat.hpp"
#include "interpolator.hpp"

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
	QByteArray logAt(const char *func) const { return QByteArray(metaObject()->className()) + "::" + func; }
protected slots:
	virtual void initializeGL();
	virtual void finalizeGL() { m_lutInt[0].delete_(); m_lutInt[1].delete_(); m_ditheringTex.delete_(); }
protected:
	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
	void setGeometryDirty() { m_dirtyGeomerty = true; }
	void setRenderTarget(const OpenGLTexture &texture) { m_texture = texture; }
	virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	virtual void prepare(QSGGeometryNode *node) = 0;
	virtual void getCoords(QRectF &vertices, QRectF &/*texCoords*/) { vertices = boundingRect(); }
	virtual QSGMaterialType *shaderId() const { return &m_types[m_interpolator->category()][m_dithering > 0]; }
	virtual TextureRendererShader *createShader() const;
private slots:
	void tryInitGL() { if (!m_init && QOpenGLContext::currentContext()) { initializeGL(); m_init = true; } }
private:
	const OpenGLTexture &texture() const { return m_texture; }
	const OpenGLTexture &ditheringTexture() const { return m_ditheringTex; }
	const Interpolator::Texture &lutInterpolatorTexture(int i) const { return m_lutInt[i]; }
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final override;
	friend class TextureRendererShader;
	struct Material;	struct Node;	struct Data; Data *d;
	bool m_dirtyGeomerty = true, m_init = false;
	const Interpolator *m_interpolator = Interpolator::get(InterpolatorType::Bilinear);
	InterpolatorType m_newInt = InterpolatorType::Bilinear;
	mutable QSGMaterialType m_types[Interpolator::CategoryMax][2];
	Interpolator::Texture m_lutInt[2];
	OpenGLTexture m_texture, m_ditheringTex;
	QQuickWindow *m_win = nullptr;
	Dithering m_dithering = Dithering::None, m_newDithering = Dithering::None;
};

class TextureRendererShader : public QSGMaterialShader {
public:
	TextureRendererShader(const TextureRendererItem *item, Interpolator::Category category = Interpolator::None, bool dithering = false);
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
	Interpolator::Category m_category = Interpolator::None;
	bool m_dithering = false;
	int m_lutCount = 0;
	int loc_lut_int[2] = {-1, -1}, loc_lut_int_mul[2] = {-1, -1};
	int loc_tex = -1, loc_vMatrix = -1, loc_dxy = -1;
	int loc_tex_size = -1;
	int loc_dithering = -1, loc_dithering_quantization = -1, loc_dithering_center = -1, loc_dithering_size = -1;
	QByteArray m_fragCode, m_vertexCode, m_forLog;
};

#endif // TEXTURERENDERERITEM_HPP
