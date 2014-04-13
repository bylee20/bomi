#ifndef GEOMETRYITEM_HPP
#define GEOMETRYITEM_HPP

#include "stdafx.hpp"
#include "openglcompat.hpp"

class GeometryItem : public QQuickItem {
	Q_OBJECT
public:
	GeometryItem(QQuickItem *parent = nullptr): QQuickItem(parent) {}
	void setGeometry(const QPointF &pos, const QSizeF &size) {setPosition(pos); setSize(size);}
	void setGeometry(const QRectF &rect) {setPosition(rect.topLeft()); setSize(rect.size());}
	QSizeF size() const { return {width(), height()}; }
	QRectF geometry() const { return {position(), size()}; }
	QRectF rect() const { return {0.0, 0.0, width(), height()}; }
protected:
	void geometryChanged(const QRectF &new_, const QRectF &old) override {
		QQuickItem::geometryChanged(new_, old);
	}
};



//template<class State>
//class OpenGLRendererMaterial : public QSGMaterial {
//public:
//private:
//	QSGMaterialType *type() const override final { return &m_type; }
//	QSGMaterialShader *createShader() const override final;
//	mutable QSGMaterialType m_type;
//};

//template<class State>
//class OpenGLRendererShader : public QSGMaterialShader {
//public:
//	virtual QList<QByteArray> attributes() const = 0;
//	virtual QByteArray vertexShaderSource() const = 0;
//	virtual QByteArray fragmentShaderSource() const = 0;
//	virtual void resolve() { }
//	virtual void update() { }
//protected:
//	void initialize() {
//		QSGMaterialShader::initialize();
//		auto prog = program();
//		m_loc_matrix = prog->uniformLocation("qt_Matrix");
//		m_loc_opacity = prog->uniformLocation("qt_Opacity");
//		resolve(program());
//	}
//	void updateState(const RenderState &state, QSGMaterial *new_, QSGMaterial *old) {
//		auto prog = program();
//		if (state.isMatrixDirty())
//			prog->setUniformValue(m_loc_matrix, state.combinedMatrix());
//		if (state.isOpacityDirty())
//			prog->setUniformValue(m_loc_opacity, state.opacity());
//		auto mate = static_cast<OpenGLRendererMaterial*>(new_);
//		auto prev = static_cast<OpenGLRendererMaterial*>(old);
//		update(mate, prev);
//	}

//private:
//	friend class OpenGLRendererMaterial;
//	const char *const *attributeNames() const override final {
//		if (m_attributes.isEmpty()) {
//			m_attrList = material->attributes();
//			m_attributes.resize(m_attrList.size());
//			for (int i = 0; i < m_attrList.size(); ++i)
//				m_attributes[i] = m_attrList[i].data();
//		}
//		return m_attributes.constData();
//	}
//	const char *fragmentShader() const override final {
//		if (m_fragmentShader.isEmpty())
//			m_fragmentShader = fragmentShaderSource();
//		return m_fragmentShader.data();
//	}
//	const char *vertexShader() const override final {
//		if (m_vertexShader.isEmpty())
//			m_vertexShader = vertexShaderSource();
//		return m_vertexShader.data();
//	}
//	QList<QByteArray> m_attrList;
//	QVector<const char*> m_attributes;
//	QByteArray m_fragmentShader, m_vertexShader;
//	int m_loc_matrix = -1, m_loc_opacity = -1;
//};

//inline QSGMaterialShader *OpenGLRendererMaterial::createShader() const {
//	return new OpenGLRendererShader(this);
//}


//class TextureRendererShader : public QSGMaterialShader {
//public:
//	TextureRendererShader(const HQTextureRendererItem *item, Interpolator::Category category = Interpolator::None, bool dithering = false, bool rectangle = false);
//	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
//	const char *fragmentShader() const override { return m_fragCode.constData(); }
//	const char *vertexShader() const override { return m_vertexCode.constData(); }
//	const char *const *attributeNames() const override;
//	virtual void link(QOpenGLShaderProgram *prog);
//	virtual void bind(QOpenGLShaderProgram *prog);
//	const HQTextureRendererItem *item() const { return m_item; }
//private:
//	void updateState(const RenderState &state, QSGMaterial */*new_*/, QSGMaterial */*old*/) final override;
//	void initialize() final override;
//	const HQTextureRendererItem *m_item = nullptr;
//	Interpolator::Category m_category = Interpolator::None;
//	bool m_dithering = false, m_rectangle = false;
//	int m_lutCount = 0;
//	int loc_lut_int[2] = {-1, -1}, loc_lut_int_mul[2] = {-1, -1};
//	int loc_tex = -1, loc_vMatrix = -1, loc_dxy = -1;
//	int loc_tex_size = -1;
//	int loc_dithering = -1, loc_dithering_quantization = -1, loc_dithering_center = -1, loc_dithering_size = -1;
//	QByteArray m_fragCode, m_vertexCode, m_forLog;
//};

class OpenGLDrawItem : public GeometryItem {
	Q_OBJECT
public:
	enum UpdateHint { UpdateGeometry = 1, UpdateMaterial = 2 };
	OpenGLDrawItem(QQuickItem *parent = nullptr);
	~OpenGLDrawItem();
	virtual void rerender() final {
		reserve(UpdateGeometry);
		reserve(UpdateMaterial);
		update();
	}
	double devicePixelRatio() const { return m_win ? m_win->devicePixelRatio() : 1.0; }
	QByteArray logAt(const char *func) const { return QByteArray(metaObject()->className()) + "::" + func; }
protected slots:
	virtual void initializeGL() { }
	virtual void finalizeGL() { }
protected:
	virtual GLenum drawingMode() const { return GL_TRIANGLE_STRIP; }
	virtual const QSGGeometry::AttributeSet &defaultAttributes() const {
		return QSGGeometry::defaultAttributes_TexturedPoint2D();
	}
	virtual int vertexCount() const { return 0; }
	virtual QSGGeometry *createGeometry() const {
		return new QSGGeometry(defaultAttributes(), vertexCount());
	}
	virtual QSGMaterial *createMaterial() const = 0;
	virtual QSGGeometry *updateGeometry(QSGGeometry *geometry) = 0;
	virtual QSGMaterial *updateMaterial(QSGMaterial *material) = 0;

	static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
	void reserve(UpdateHint update) { m_updates |= update; }
	bool isReserved(UpdateHint update) const { return m_updates & update; }
private:
	void tryInitGL() { if (!m_init && QOpenGLContext::currentContext()) { initializeGL(); m_init = true; } }
	QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final override;
	QQuickWindow *m_win = nullptr;
	bool m_init = false;
	QSGGeometryNode *m_node = nullptr;
	int m_updates = 0;
};

class SimpleTextureItem : public OpenGLDrawItem {
	Q_OBJECT
public:
	struct Attribute { QPointF vertex, texCoord; };
	SimpleTextureItem(QQuickItem *parent = nullptr);
	void setTexture(const OpenGLTexture2D &texture);
	void setAttributes(const QVector<Attribute> &attrs);
	OpenGLTexture2D &texture() { return m_texture; }
	const OpenGLTexture2D &texture() const { return m_texture; }
	const QVector<Attribute> &attributes() const { return m_attrs; }
	QVector<Attribute> &attributes() { return m_attrs; }
protected:
	virtual void updateTexture(OpenGLTexture2D &texture) { Q_UNUSED(texture); }
private:
	const QSGGeometry::AttributeSet &defaultAttributes() const override final {
		return QSGGeometry::defaultAttributes_TexturedPoint2D();
	}
	QSGMaterial *createMaterial() const override final;
	QSGGeometry *updateGeometry(QSGGeometry *geometry) override final;
	QSGMaterial *updateMaterial(QSGMaterial *material) override final;

	OpenGLTexture2D m_texture;
	QVector<Attribute> m_attrs;
};


#endif // GEOMETRYITEM_HPP
