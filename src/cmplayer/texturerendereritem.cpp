#include "texturerendereritem.hpp"
#include "enums.hpp"
#include "global.hpp"
#include "openglcompat.hpp"

static const char *code = R"(

varying vec2 texCoord;
#if USE_INTERPOLATOR
varying vec2 lutIntCoord;
uniform vec2 dxy;
#endif

/***********************************************************************/

#ifdef FRAGMENT
uniform sampler2D tex;

#if USE_INTERPOLATOR
uniform sampler1D lut_int1;
uniform float lut_int1_mul;
#if HAS_FLOAT_TEXTURE
#define renormalize(a, b) (a)
#else
vec4 renormalize(const in vec4 v, float mul) {
	return v*mul-0.5*mul;
}
#endif
#if USE_INTERPOLATOR == 2
uniform sampler1D lut_int2;
uniform float lut_int2_mul;
vec4 mix3(const in vec4 v1, const in vec4 v2, const in vec4 v3, const in float a, const in float b) {
	return mix(mix(v1, v2, a), v3, b);
}
#endif
#endif

vec4 interpolated(const in vec2 coord) {
#if USE_INTERPOLATOR == 0
	return texture2D(tex, coord);
#elif USE_INTERPOLATOR == 1
	// b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
	vec4 hg_x = renormalize(texture1D(lut_int1, lutIntCoord.x), lut_int1_mul);
	vec4 hg_y = renormalize(texture1D(lut_int1, lutIntCoord.y), lut_int1_mul);

	vec4 tex00 = texture2D(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
	vec4 tex10 = texture2D(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
	vec4 tex01 = texture2D(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
	vec4 tex11 = texture2D(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

	tex00 = mix(tex00, tex10, hg_x.a);
	tex01 = mix(tex01, tex11, hg_x.a);
	return  mix(tex00, tex01, hg_y.a);
#elif USE_INTERPOLATOR == 2
	vec4 h_x = renormalize(texture1D(lut_int1, lutIntCoord.x), lut_int1_mul);
	vec4 h_y = renormalize(texture1D(lut_int1, lutIntCoord.y), lut_int1_mul);
	vec4 f_x = renormalize(texture1D(lut_int2, lutIntCoord.x), lut_int2_mul);
	vec4 f_y = renormalize(texture1D(lut_int2, lutIntCoord.y), lut_int2_mul);

	vec4 tex00 = texture2D(tex, coord + vec2(-h_x.b, -h_y.b)*dxy);
	vec4 tex01 = texture2D(tex, coord + vec2(-h_x.b, -h_y.g)*dxy);
	vec4 tex02 = texture2D(tex, coord + vec2(-h_x.b,  h_y.r)*dxy);
	tex00 = mix3(tex00, tex01, tex02, f_y.b, f_y.g);

	vec4 tex10 = texture2D(tex, coord + vec2(-h_x.g, -h_y.b)*dxy);
	vec4 tex11 = texture2D(tex, coord + vec2(-h_x.g, -h_y.g)*dxy);
	vec4 tex12 = texture2D(tex, coord + vec2(-h_x.g,  h_y.r)*dxy);
	tex10 = mix3(tex10, tex11, tex12, f_y.b, f_y.g);

	vec4 tex20 = texture2D(tex, coord + vec2( h_x.r, -h_y.b)*dxy);
	vec4 tex21 = texture2D(tex, coord + vec2( h_x.r, -h_y.g)*dxy);
	vec4 tex22 = texture2D(tex, coord + vec2( h_x.r,  h_y.r)*dxy);
	tex20 = mix3(tex20, tex21, tex22, f_y.b, f_y.g);
	return mix3(tex00, tex10, tex20, f_x.b, f_x.g);
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
	lutIntCoord = vCoord/dxy - vec2(0.5, 0.5);
#endif
	texCoord = vCoord;
	gl_Position = vMatrix * vPosition;
}
#endif

)";

TextureRendererShader::TextureRendererShader(const TextureRendererItem *item, int interpolator, bool dithering)
: m_item(item), m_interpolator(interpolator), m_dithering(dithering) {
	QByteArray header;
	header += "#define HAS_FLOAT_TEXTURE " + QByteArray::number(OpenGLCompat::hasFloat()) + "\n";
	header += "#define USE_INTERPOLATOR " + QByteArray::number(m_interpolator) + "\n";
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
	int texPos = 0;
	if (m_interpolator) {
		prog->setUniformValue(loc_lut_int1, ++texPos);
		prog->setUniformValue(loc_dxy, QVector2D(1.f/(float)texture.width, 1.f/(float)texture.height));
		prog->setUniformValue(loc_lut_int1_mul, m_item->lutInterpolatorTexture1().multiply);
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		m_item->lutInterpolatorTexture1().bind();
		if (m_interpolator > 1) {
			prog->setUniformValue(loc_lut_int2, ++texPos);
			prog->setUniformValue(loc_lut_int2_mul, m_item->lutInterpolatorTexture2().multiply);
			f->glActiveTexture(GL_TEXTURE0 + texPos);
			m_item->lutInterpolatorTexture2().bind();
		}
	}
	if (m_dithering) {
		auto &dithering = m_item->ditheringTexture();
		Q_ASSERT(dithering.width == dithering.height);
		Q_ASSERT(loc_dithering != -1);
		Q_ASSERT(loc_dithering_quantization != -1);
		Q_ASSERT(loc_dithering_center != -1);
		Q_ASSERT(loc_dithering_size != -1);
		const int size = dithering.width;
		prog->setUniformValue(loc_dithering, ++texPos);
		prog->setUniformValue(loc_dithering_quantization, float(1 << m_item->depth()) - 1.f);
		prog->setUniformValue(loc_dithering_center, 0.5f / size*size);
		prog->setUniformValue(loc_dithering_size, size);
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		dithering.bind();
	}
	f->glActiveTexture(GL_TEXTURE0);
}

void TextureRendererShader::initialize() {
	auto prog = program();
	loc_vMatrix = prog->uniformLocation("vMatrix");
	loc_tex = prog->uniformLocation("tex");
	if (m_interpolator) {
		loc_lut_int1 = prog->uniformLocation("lut_int1");
		loc_lut_int1_mul = prog->uniformLocation("lut_int1_mul");
		loc_dxy = prog->uniformLocation("dxy");
		if (m_interpolator > 1) {
			loc_lut_int2 = prog->uniformLocation("lut_int2");
			loc_lut_int2_mul = prog->uniformLocation("lut_int2_mul");
		}
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
	return new TextureRendererShader(this, m_intCategory, m_dithering > 0);
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
	if (_Change(m_interpolator, m_newInt)) {
		m_intCategory = interpolatorCategory(m_interpolator);
		OpenGLCompat::allocateInterpolatorLutTexture(m_lutInt1, m_lutInt2, m_interpolator);
		if (m_interpolator > 0)
			qDebug() << InterpolatorTypeInfo::name(m_interpolator);
	}
	if (_Change(m_dithering, m_newDithering))
		m_ditheringTex = OpenGLCompat::allocateDitheringTexture(m_ditheringTex.id, m_dithering);
	return d->node;
}

void TextureRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	GeometryItem::geometryChanged(newOne, old);
	m_dirtyGeomerty = true;
	update();
}
