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
	QSGMaterialType *type() const;
	QSGMaterialShader *createShader() const;
private:
	TextureRendererItem *m_item = nullptr;
	mutable QSGMaterialType m_type;
};

QSGMaterialType *TextureRendererItem::Material::type() const {return &m_type;}
QSGMaterialShader *TextureRendererItem::Material::createShader() const {return new Shader(m_item);}

struct TextureRendererItem::Node : public QSGGeometryNode {
	Node(TextureRendererItem *item): m_item(item) {
		setFlags(OwnsGeometry | OwnsMaterial);
		setMaterial(new Material(item));
		setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
		markDirty(DirtyMaterial | DirtyGeometry);

		item->setGeometryDirty();
		Q_ASSERT(!item->m_textures);
		item->m_textures = new GLuint[item->textureCount()];
		glGenTextures(item->textureCount(), item->m_textures);
		item->initializeTextures();
	}
	~Node() {
		if (m_item->m_textures) {
			Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
			glDeleteTextures(m_item->textureCount(), m_item->m_textures);
			m_item->m_textures = nullptr;
		}
	}
private:
	TextureRendererItem *m_item;
};

struct TextureRendererItem::Data {
	Node *node = nullptr;
	bool dirtyGeomerty = false;
	int loc_matrix = 0;
};

TextureRendererItem::TextureRendererItem(int textures, QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	setFlag(ItemHasContents);
	m_count = textures;
}

TextureRendererItem::~TextureRendererItem() {
	delete d;
}

void TextureRendererItem::setGeometryDirty(bool dirty) {
	d->dirtyGeomerty = dirty;
}

void TextureRendererItem::resetNode() {
	if (d->node) {
		delete d->node;
		d->node = nullptr;
	}
	if (!d->node)
		d->node = new Node(this);
}

QSGNode *TextureRendererItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	d->node = static_cast<Node*>(old);
	if (!d->node)
		d->node = new Node(this);
	beforeUpdate();
	if (d->dirtyGeomerty) {
		updateTexturedPoint2D(d->node->geometry()->vertexDataAsTexturedPoint2D());
		d->node->markDirty(QSGNode::DirtyGeometry);
		d->dirtyGeomerty = false;
	}
	return d->node;
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
