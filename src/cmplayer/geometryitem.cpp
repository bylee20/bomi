#include "geometryitem.hpp"

OpenGLDrawItem::OpenGLDrawItem(QQuickItem *parent)
: GeometryItem(parent) {
	setFlag(ItemHasContents, true);
	connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow *window) {
		m_win = window;
		if (window) {
			connect(window, &QQuickWindow::sceneGraphInitialized, this, &OpenGLDrawItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::beforeRendering, this, &OpenGLDrawItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::sceneGraphInvalidated, this, &OpenGLDrawItem::finalizeGL, Qt::DirectConnection);
		}
	});
}

OpenGLDrawItem::~OpenGLDrawItem() {

}

QSGNode *OpenGLDrawItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	tryInitGL();
	Q_UNUSED(data);
	m_node = static_cast<QSGGeometryNode*>(old);
	if (!m_node) {
		m_node = new QSGGeometryNode;
		m_node->setGeometry(createGeometry());
		m_node->setMaterial(createMaterial());
		m_node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
		m_node->geometry()->setDrawingMode(drawingMode());
		reserve(UpdateMaterial);
		reserve(UpdateGeometry);
	}
	if (isReserved(UpdateMaterial)) {
		auto m = m_node->material();
		if (_Change(m, updateMaterial(m)))
			m_node->setMaterial(m);
		m_node->markDirty(QSGNode::DirtyMaterial);
	}
	if (isReserved(UpdateGeometry)) {
		auto g = m_node->geometry();
		if (_Change(g, updateGeometry(g)))
			m_node->setGeometry(g);
		m_node->markDirty(QSGNode::DirtyGeometry);
	}
	m_updates = 0;
	return m_node;
}


struct TextureState {
	const OpenGLTexture2D *texture = nullptr;
};

struct SimpleTextureShader : public QSGSimpleMaterialShader<TextureState> {
	QSG_DECLARE_SIMPLE_SHADER(SimpleTextureShader, TextureState)
	const char *vertexShader() const override {
		static const char *code = R"(
			uniform mat4 qt_Matrix;
			attribute vec4 aVertex;
			attribute vec2 aTexCoord;
			varying vec2 texCoord;
			void main() {
				texCoord = aTexCoord;
				gl_Position = qt_Matrix * aVertex;
			}
		)";
		return code;
	}
	const char *fragmentShader() const override {
		static const char *code = R"(
			varying vec2 texCoord;
			uniform float qt_Opacity;
			uniform sampler2D tex;
			void main() {
				gl_FragColor = texture2D(tex, texCoord) * qt_Opacity;
			}
		)";
		return code;
	}
	QList<QByteArray> attributes() const override {
		return QList<QByteArray>() << "aVertex" << "aTexCoord";
	}
	void resolveUniforms() {
		loc_tex = program()->uniformLocation("tex");
	}
	void updateState(const TextureState *new_, const TextureState *) override {
		program()->setUniformValue(loc_tex, 0);
		glActiveTexture(GL_TEXTURE0);
		new_->texture->bind();
	}
private:
	int loc_tex = -1;
};

using SimpleTextureMaterial = QSGSimpleMaterial<TextureState>;

SimpleTextureItem::SimpleTextureItem(QQuickItem *parent)
: OpenGLDrawItem(parent) {

}

void SimpleTextureItem::setTexture(const OpenGLTexture2D &texture) {
	m_texture = texture;
	reserve(UpdateMaterial);
}

QSGMaterial *SimpleTextureItem::createMaterial() const {
	auto m = SimpleTextureShader::createMaterial();
	m->state()->texture = &m_texture;
	m->setFlag(QSGMaterial::Blending);
	return m;
}

QSGMaterial *SimpleTextureItem::updateMaterial(QSGMaterial *material) {
	auto m = static_cast<SimpleTextureMaterial*>(material);
	Q_ASSERT(m->state()->texture == &m_texture);
	updateTexture(m_texture);
	return m;
}

QSGGeometry *SimpleTextureItem::updateGeometry(QSGGeometry *geometry) {
	geometry->allocate(m_attrs.size());
	auto p = geometry->vertexDataAsTexturedPoint2D();
	for (auto &attr : m_attrs) {
		p->x  = attr.vertex.x();
		p->y  = attr.vertex.y();
		p->tx = attr.texCoord.x();
		p->ty = attr.texCoord.y();
		++p;
	}
	return geometry;
}

void SimpleTextureItem::setAttributes(const QVector<Attribute> &attrs) {
	m_attrs = attrs;
	reserve(UpdateGeometry);
}
