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

#if USE_DITHERing
uniform sampler2D dithering;
uniform float dithering_quantization;
uniform float dithering_center;
uniform vec2 dithering_size;
vec4 ditheringed(const in vec4 color) {
	vec2 dithering_pos = texCoord.xy / dithering_size;
	float dithering_value = texture2D(dithering, dithering_pos).r + dithering_center;
	return floor(color * dithering_quantization + dithering_value) / dithering_quantization;
}
#endif

void main() {
	vec4 color = interpolated(texCoord);
#if USE_DITHERing
	color = ditheringed(color);
#endif
	gl_FragColor = color;
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

TextureRendererShader::TextureRendererShader(const TextureRendererItem *item, bool interpolator, bool dithering)
	: m_item(item), m_interpolator(interpolator), m_dithering(dithering) {
	QByteArray header;
	header += "#define USE_INTERPOLATOR " + QByteArray::number(interpolator) + "\n";
	header += "#define USE_DITHERing " + QByteArray::number(dithering) + "\n";
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
	}
	if (m_dithering) {
		auto &dithering = m_item->ditheringTexture();
		Q_ASSERT(dithering.width == dithering.height);
		Q_ASSERT(loc_dithering != -1);
		Q_ASSERT(loc_dithering_quantization != -1);
		Q_ASSERT(loc_dithering_center != -1);
		Q_ASSERT(loc_dithering_size != -1);
		const int size = dithering.width;
		prog->setUniformValue(loc_dithering, 2);
		prog->setUniformValue(loc_dithering_quantization, float(1 << m_item->depth()) - 1.f);
		prog->setUniformValue(loc_dithering_center, 0.5f / size*size);
		prog->setUniformValue(loc_dithering_size, size);
		f->glActiveTexture(GL_TEXTURE2);
		dithering.bind();
	}
	f->glActiveTexture(GL_TEXTURE0);
}

void TextureRendererShader::initialize() {
	auto prog = program();
	loc_vMatrix = prog->uniformLocation("vMatrix");
	loc_tex = prog->uniformLocation("tex");
	if (m_interpolator) {
		loc_lut_interpolator = prog->uniformLocation("lut_interpolator");
		loc_dxy = prog->uniformLocation("dxy");
	}
	if (m_dithering) {
		loc_dithering = prog->uniformLocation("dithering");
		loc_dithering_quantization = prog->uniformLocation("dithering_quantization");
		loc_dithering_center = prog->uniformLocation("dithering_center");
		loc_dithering_size = prog->uniformLocation("dithering_size");
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
		m_win = window;
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
	return new TextureRendererShader(this, m_interpolator > 0, m_dithering > 0);
}

QSGNode *TextureRendererItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	tryInitGL();
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
	if (_Change(m_dithering, m_newDithering))
		m_ditheringTex = OpenGLCompat::allocateDitheringTexture(m_ditheringTex.id, m_dithering);
	return d->node;
}

void TextureRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	GeometryItem::geometryChanged(newOne, old);
	m_dirtyGeomerty = true;
	update();
}
