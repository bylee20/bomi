#include "texturerendereritem.hpp"

struct TextureRendererItem::Shader : public QSGMaterialShader {
	Shader(TextureRendererItem *item): m_item(item) {}
	void updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
		Q_UNUSED(old);
		Q_UNUSED(newOne);
		m_item->bind(state, program());
	}
	void initialize() {
		m_item->link(program());
	}
private:
	char const *const *attributeNames() const {return m_item->attributeNames();}
	const char *vertexShader() const {return m_item->vertexShader();}
	const char *fragmentShader() const {return m_item->fragmentShader();}
	TextureRendererItem *m_item = nullptr;
};

struct TextureRendererItem::Material : public QSGMaterial {
	Material(TextureRendererItem *item): m_item(item) {if (item->blending()) setFlag(Blending);}
	QSGMaterialType *type() const {return &m_type;}
	QSGMaterialShader *createShader() const {qDebug() << "new shader"; return new Shader(m_item);}
private:
	TextureRendererItem *m_item = nullptr;
	mutable QSGMaterialType m_type;
};

struct TextureRendererItem::Node : public QSGGeometryNode {
	Node(TextureRendererItem *item) {
		setFlags(OwnsGeometry | OwnsMaterial);
		setMaterial(new Material(item));
		setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
	}
private:
};

struct TextureRendererItem::Data {
	bool frameChanged = false;
	bool dirtyGeomerty = false;
	QRectF contentRect;
	QRectF vtx;
	QPoint offset = {0, 0};
	int loc_matrix = 0;
	int texCount = 0;
};

TextureRendererItem::TextureRendererItem(int textures, QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	setFlag(ItemHasContents);
	d->texCount = textures;
}

TextureRendererItem::~TextureRendererItem() {
	if (m_textures) {
		glDeleteTextures(d->texCount, m_textures);
		delete []m_textures;
	}
	delete d;
}

void TextureRendererItem::setGeometryDirty(bool dirty) {
	d->dirtyGeomerty = dirty;
}

QSGNode *TextureRendererItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	if (!m_textures) {
		m_textures = new GLuint[d->texCount];
		glGenTextures(3, m_textures);
	}
	auto node = static_cast<Node*>(old);
	const bool reset = beforeUpdate();
	if (node && reset) {
		delete node;
		node = nullptr;
	}
	if (!node) {
		qDebug() << "new node";
		node = new Node(this);
		d->dirtyGeomerty = true;
	}
	if (d->dirtyGeomerty) {
		updateTexturedPoint2D(node->geometry()->vertexDataAsTexturedPoint2D());
		node->markDirty(QSGNode::DirtyGeometry);
		d->dirtyGeomerty = false;
	}
	return node;
}

void TextureRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	QQuickItem::geometryChanged(newOne, old);
	d->dirtyGeomerty = true;
	update();
}

char const *const *TextureRendererItem::attributeNames() const {
	static const char *names[] = {
		"qt_VertexPosition",
		"qt_VertexTexCoord",
		0
	};
	return names;
}

const char *TextureRendererItem::vertexShader() const {
	static const char *shader = (R"(
		uniform highp mat4 qt_Matrix;
		attribute highp vec4 qt_VertexPosition;
		attribute highp vec2 qt_VertexTexCoord;
		varying highp vec2 qt_TexCoord;
		void main() {
			qt_TexCoord = qt_VertexTexCoord;
			gl_Position = qt_Matrix * qt_VertexPosition;
		}
	)");
	return shader;
}

void TextureRendererItem::link(QOpenGLShaderProgram *program) {
	d->loc_matrix = program->uniformLocation("qt_Matrix");
}

void TextureRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	if (state.isMatrixDirty())
		program->setUniformValue(d->loc_matrix, state.combinedMatrix());
}
