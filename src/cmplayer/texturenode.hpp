//#ifndef TEXTURENODE_HPP
//#define TEXTURENODE_HPP

//#include "stdafx.hpp"

//class TextureItem : public QQuickItem {
//	Q_OBJECT
//public:
//	TextureItem(Q)
//protected:

//};

//class TextureNode : public QSGGeometryNode {
//public:
//	TextureNode(bool opaque = true);
//	~TextureNode();
//	virtual QVector<GLuint> textures() const = 0;
//	virtual void bindTexture() = 0;
//	virtual QSGMaterialShader *createShader();
//	virtual QSGMaterialType *type() const;
//	bool isOpaque() const { return m_opaque; }
//	void updateGeometry(const QRectF &vertices
//		, const QRectF &texCoords = {0.0, 0.0, 1.0, 1.0});
//	void updateGeometry(const QPointF &vtxTL, const QPointF &vtxBR
//		, const QPointF &txtTL = {0.0, 0.0}, const QPointF &txtBR = {1.0, 1.0});
//private:
//	class Material;
//	mutable QSGMaterialType m_type;
//	bool m_opaque = false;
//};

//class TextureNodeShader : public QSGMaterialShader {
//public:
//	TextureNodeShader(TextureNode *node): m_node(node) {}
//	virtual void bind(QOpenGLShaderProgram *prog);
//	virtual void link(QOpenGLShaderProgram *prog);
//	virtual const char *const *attributeNames() const override;
//	virtual const char *vertexShader() const override;
//	virtual const char *fragmentShader() const override;
//private:
//	void updateState(const RenderState &state, QSGMaterial *new_, QSGMaterial *old) final override;
//	void initialize() final override;
//	int loc_vMatrix = 0, loc_tex = 0;
//	TextureNode *m_node = nullptr;
//};

//#endif // TEXTURENODE_HPP
