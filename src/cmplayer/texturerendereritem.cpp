#include "texturerendereritem.hpp"
#include "enums.hpp"
#include "global.hpp"
#include "openglcompat.hpp"

static const char *code = R"(

varying vec2 texCoord;
#if USE_INTERPOLATOR
varying vec2 lutInterpolatorCoord;
uniform vec2 dxy;
#endif

/***********************************************************************/

#ifdef FRAGMENT
uniform sampler2D tex;
#if USE_INTERPOLATOR
uniform sampler1D lut_interpolator;
#endif
vec4 interpolated(const in vec2 coord) {
#if USE_INTERPOLATOR
	// b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
	vec4 hg_x = texture1D(lut_interpolator, lutInterpolatorCoord.x);
	vec4 hg_y = texture1D(lut_interpolator, lutInterpolatorCoord.y);

	vec4 tex00 = texture2D(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
	vec4 tex10 = texture2D(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
	vec4 tex01 = texture2D(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
	vec4 tex11 = texture2D(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

	tex00 = mix(tex00, tex10, hg_x.a);
	tex01 = mix(tex01, tex11, hg_x.a);
	return  mix(tex00, tex01, hg_y.a);
#else
	return texture2D(tex, coord);
#endif
}

void main() {
	gl_FragColor = interpolated(texCoord);
}

#endif
/***********************************************************************/

#ifdef VERTEX
uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;
void main() {
#if USE_INTERPOLATOR
	lutInterpolatorCoord = vCoord/dxy - vec2(0.5, 0.5);
#endif
	texCoord = vCoord;
	gl_Position = vMatrix * vPosition;
}
#endif

)";

TextureRendererShader::TextureRendererShader(const TextureRendererItem *item, bool interpolator)
: m_item(item), m_interpolator(interpolator) {
	const QByteArray header("#define USE_INTERPOLATOR " + QByteArray::number(interpolator) + "\n");
	m_fragCode = header;
	m_fragCode += "#define FRAGMENT\n";
	m_fragCode += code;
	m_vertexCode = header;
	m_vertexCode += "#define VERTEX\n";
	m_vertexCode += code;
}

const char *const *TextureRendererShader::attributeNames() const {
	static const char *const names[] = {"vPosition", "vCoord", nullptr};
	return names;
}

void TextureRendererShader::link(QOpenGLShaderProgram */*prog*/) { }
void TextureRendererShader::bind(QOpenGLShaderProgram */*prog*/) { }

void TextureRendererShader::updateState(const RenderState &state, QSGMaterial */*new_*/, QSGMaterial */*old*/) {
	auto &texture = m_item->texture();
	auto prog = program();
	auto f = func();

	prog->setUniformValue(loc_tex, 0);
	if (state.isMatrixDirty())
		prog->setUniformValue(loc_vMatrix, state.combinedMatrix());
	bind(prog);
	f->glActiveTexture(GL_TEXTURE0);
	texture.bind();
	if (m_interpolator) {
		prog->setUniformValue(loc_lut_interpolator, 1);
		prog->setUniformValue(loc_dxy, QVector2D(1.f/(float)texture.width, 1.f/(float)texture.height));
		f->glActiveTexture(GL_TEXTURE1);
		m_item->lutInterpolatorTexture().bind();
		f->glActiveTexture(GL_TEXTURE0);
	}
}

void TextureRendererShader::initialize() {
	auto prog = program();
	loc_vMatrix = prog->uniformLocation("vMatrix");
	loc_tex = prog->uniformLocation("tex");
	if (m_interpolator) {
		loc_lut_interpolator = prog->uniformLocation("lut_interpolator");
		loc_dxy = prog->uniformLocation("dxy");
	}
	link(prog);
}

struct TextureRendererItem::Material : public QSGMaterial {
	Material(TextureRendererItem *item): m_item(item) { setFlag(Blending, !item->isOpaque()); }
	QSGMaterialType *type() const { return m_item->shaderId(); }
	QSGMaterialShader *createShader() const { return m_item->createShader(); }
private:
    TextureRendererItem *m_item = nullptr;
};

struct TextureRendererItem::Node : public QSGGeometryNode {
	Node(TextureRendererItem *item) {
		setFlags(OwnsGeometry | OwnsMaterial);
        setMaterial(new Material(item));
		setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4*item->quads()));
		markDirty(DirtyMaterial | DirtyGeometry);
	}
};

struct TextureRendererItem::Data {
	Node *node = nullptr;
	static void set(QSGGeometry::TexturedPoint2D *tp, const QRectF &vtx, const QRectF &txt) {
		auto set = [&tp] (const QPointF &vtx, const QPointF &txt) {
			tp++->set(vtx.x(), vtx.y(), txt.x(), txt.y());
		};
		set(vtx.topLeft(), txt.topLeft());
		set(vtx.bottomLeft(), txt.bottomLeft());
		set(vtx.topRight(), txt.topRight());
		set(vtx.bottomRight(), txt.bottomRight());
	}
};

TextureRendererItem::TextureRendererItem(QQuickItem *parent)
: GeometryItem(parent), d(new Data) {
	setFlag(ItemHasContents, true);
	connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow *window) {
		if (window) {
			connect(window, &QQuickWindow::sceneGraphInitialized, this, &TextureRendererItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::beforeRendering, this, &TextureRendererItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::sceneGraphInvalidated, this, &TextureRendererItem::finalizeGL, Qt::DirectConnection);
		}
	});
}

TextureRendererItem::~TextureRendererItem() {
	delete d;
}

TextureRendererShader *TextureRendererItem::createShader() const {
	return new TextureRendererShader(this, m_interpolator > 0);
}

QSGNode *TextureRendererItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	d->node = static_cast<Node*>(old);
	if (!d->node)
		d->node = new Node(this);
	prepare(d->node);
	if (m_dirtyGeomerty) {
		QRectF vtx, txt{0.0, 0.0, 1.0, 1.0}; getCoords(vtx, txt);
		d->set(d->node->geometry()->vertexDataAsTexturedPoint2D(), vtx, txt);
		d->node->markDirty(QSGNode::DirtyGeometry);
		m_dirtyGeomerty = false;
	}
	if (_Change(m_interpolator, m_newInt))
		m_lutInt = OpenGLCompat::allocateInterpolatorLutTexture(m_lutInt.id, m_interpolator);
	return d->node;
}

void TextureRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	GeometryItem::geometryChanged(newOne, old);
	m_dirtyGeomerty = true;
	update();
}
