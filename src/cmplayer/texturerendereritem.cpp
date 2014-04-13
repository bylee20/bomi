#include "texturerendereritem.hpp"
#include "enums.hpp"
#include "global.hpp"
#include "openglcompat.hpp"
extern "C" {
#include <video/out/dither.h>
}

static const char *head = R"(
varying vec2 texCoord;
#define DEC_UNIFORM_DXY
#if USE_RECTANGLE
varying vec2 normalizedTexCoord;
#define sampler2Dg sampler2DRect
#define texture2Dg texture2DRect
#else
#define normalizedTexCoord texCoord
#define sampler2Dg sampler2D
#define texture2Dg texture2D
#endif
)";

static const char *fragCode = R"(
uniform sampler2Dg tex;
#if USE_DITHERING
uniform sampler2D dithering;
uniform float dithering_quantization;
uniform float dithering_center;
uniform vec2 dithering_size;
vec4 ditheringed(const in vec4 color) {
	vec2 dithering_pos = normalizedTexCoord.xy / dithering_size;
	float dithering_value = texture2D(dithering, dithering_pos).r + dithering_center;
	return floor(color * dithering_quantization + dithering_value) / dithering_quantization;
}
#endif
void main() {
	vec4 color = interpolated(tex, texCoord);
#if USE_DITHERING
	color = ditheringed(color);
#endif
	gl_FragColor = color;
}
)";

static const char *vtxCode = R"(
uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;
void main() {
	setLutIntCoord(vCoord);
	texCoord = vCoord;
#if (USE_RECTANGLE && USE_DITHERING)
	normalizedTexCoord = vCoord/tex_size;
#endif
	gl_Position = vMatrix * vPosition;
}
)";

TextureRendererShader::TextureRendererShader(const HQTextureRendererItem *item, Interpolator::Category category, bool dithering, bool rectangle)
: m_item(item), m_category(category), m_dithering(dithering), m_rectangle(rectangle) {
	const auto interpolator = Interpolator::shader(m_category);
	QByteArray header;
	header += "#define USE_DITHERING " + QByteArray::number(dithering) + "\n";
	header += "#define USE_RECTANGLE " + QByteArray::number(rectangle) + "\n";
	header += head;
	m_fragCode = header;
	m_fragCode += "#define FRAGMENT\n";
	m_fragCode += interpolator;
	m_fragCode += fragCode;
	m_vertexCode = header;
	m_vertexCode += "#define VERTEX\n";
	m_vertexCode += interpolator;
	m_vertexCode += vtxCode;
	m_lutCount = Interpolator::textures(m_category);
	Q_ASSERT(0 <= m_lutCount && m_lutCount < 3);
	LOG_GL_ERROR
}

const char *const *TextureRendererShader::attributeNames() const {
	static const char *const names[] = {"vPosition", "vCoord", nullptr};
	return names;
}

void TextureRendererShader::link(QOpenGLShaderProgram */*prog*/) { }
void TextureRendererShader::bind(QOpenGLShaderProgram */*prog*/) { }

void TextureRendererShader::updateState(const RenderState &state, QSGMaterial */*new_*/, QSGMaterial */*old*/) {
	auto &texture = m_item->renderTarget();
	auto prog = program();
	auto f = func();
	prog->setUniformValue(loc_tex, 0);
	if (state.isMatrixDirty())
		prog->setUniformValue(loc_vMatrix, state.combinedMatrix());
	bind(prog);
	f->glActiveTexture(GL_TEXTURE0);
	texture.bind();
	if (m_rectangle)
		prog->setUniformValue(loc_dxy, QVector2D(1.0, 1.0));
	else
		prog->setUniformValue(loc_dxy, QVector2D(1.0/(double)texture.width(), 1.0/(double)texture.height()));
	prog->setUniformValue(loc_tex_size, QVector2D(texture.width(), texture.height()));

	auto texPos = 1;
	for (int i=0; i<m_lutCount; ++i, ++texPos) {
		prog->setUniformValue(loc_lut_int[i], texPos);
		prog->setUniformValue(loc_lut_int_mul[i], m_item->lutInterpolatorTexture(i).multiplier());
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		m_item->lutInterpolatorTexture(i).bind();
	}
	if (m_dithering) {
		auto &dithering = m_item->ditheringTexture();
		Q_ASSERT(dithering.width() == dithering.height());
		const int size = dithering.width();
		prog->setUniformValue(loc_dithering, texPos);
		prog->setUniformValue(loc_dithering_quantization, float(1 << m_item->depth()) - 1.f);
		prog->setUniformValue(loc_dithering_center, 0.5f / size*size);
		prog->setUniformValue(loc_dithering_size, QSize(size, size));
		f->glActiveTexture(GL_TEXTURE0 + texPos);
		dithering.bind();
	}
	f->glActiveTexture(GL_TEXTURE0);
	OpenGLCompat::logError(m_forLog.constData());
}

void TextureRendererShader::initialize() {
	auto prog = program();
	loc_vMatrix = prog->uniformLocation("vMatrix");
	loc_tex = prog->uniformLocation("tex");
	if (m_lutCount > 0) {
		loc_dxy = prog->uniformLocation("dxy");
		loc_tex_size = prog->uniformLocation("tex_size");
	}
	for (int i=0; i<m_lutCount; ++i) {
		auto name = QByteArray("lut_int") + QByteArray::number(i+1);
		loc_lut_int[i] = prog->uniformLocation(name);
		loc_lut_int_mul[i] = prog->uniformLocation(name + "_mul");
	}
	if (m_dithering) {
		loc_dithering = prog->uniformLocation("dithering");
		loc_dithering_quantization = prog->uniformLocation("dithering_quantization");
		loc_dithering_center = prog->uniformLocation("dithering_center");
		loc_dithering_size = prog->uniformLocation("dithering_size");
		Q_ASSERT(loc_dithering != -1);
		Q_ASSERT(loc_dithering_quantization != -1);
		Q_ASSERT(loc_dithering_center != -1);
		Q_ASSERT(loc_dithering_size != -1);
	}
	link(prog);
	LOG_GL_ERROR
	m_forLog = m_item->metaObject()->className();
	m_forLog += "->TextureRendererShader::";
	const auto at = m_forLog + "initialize()";
	OpenGLCompat::logError(at.constData());
	m_forLog += "updateState()";
}

struct HQTextureRendererItem::Material : public QSGMaterial {
	Material(HQTextureRendererItem *item): m_item(item) { setFlag(Blending, !item->isOpaque()); }
	QSGMaterialType *type() const { return m_item->shaderId(); }
	QSGMaterialShader *createShader() const { return m_item->createShader(); }
private:
	HQTextureRendererItem *m_item = nullptr;
};

struct HQTextureRendererItem::Node : public QSGGeometryNode {
	Node(HQTextureRendererItem *item) {
		setFlags(OwnsGeometry | OwnsMaterial);
        setMaterial(new Material(item));
		setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4*item->quads()));
		markDirty(DirtyMaterial | DirtyGeometry);
	}
};

struct HQTextureRendererItem::Data {
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

HQTextureRendererItem::HQTextureRendererItem(QQuickItem *parent)
: GeometryItem(parent), d(new Data) {

	setFlag(ItemHasContents, true);
	connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow *window) {
		m_win = window;
		if (window) {
			connect(window, &QQuickWindow::sceneGraphInitialized, this, &HQTextureRendererItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::beforeRendering, this, &HQTextureRendererItem::tryInitGL, Qt::DirectConnection);
			connect(window, &QQuickWindow::sceneGraphInvalidated, this, &HQTextureRendererItem::finalizeGL, Qt::DirectConnection);
		}
	});
}

HQTextureRendererItem::~HQTextureRendererItem() {
	delete d;
}

QSGMaterialType *HQTextureRendererItem::shaderId() const {
	return &m_types[m_interpolator->category()][m_dithering > 0][m_texture.isRectangle()];
}

TextureRendererShader *HQTextureRendererItem::createShader() const {
	return new TextureRendererShader(this, m_interpolator->category(), m_dithering > 0, m_texture.isRectangle());
}

void HQTextureRendererItem::initializeGL() {
	m_lutInt[0].create();
	m_lutInt[1].create();
	m_ditheringTex.create(OGL::Nearest, OGL::Repeat);
	m_init = true;
	//	 gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//	 gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

static void makeDitheringTexture(OpenGLTexture2D &texture, Dithering type) {
	if (type == Dithering::None)
		return;
	const int sizeb = 6;
	int size = 0;
	const void *data = nullptr;
	QByteArray buffer;
	OpenGLTextureTransferInfo info;
	if (type == Dithering::Fruit) {
		size = 1 << 6;
		static QVector<GLfloat> fruit;
		if (fruit.size() != size*size) {
			fruit.resize(size*size);
			mp_make_fruit_dither_matrix(fruit.data(), sizeb);
		}
		const bool rg = OpenGLCompat::hasExtension(OpenGLCompat::TextureRG);
		info.texture = rg ? OGL::R16_UNorm : OGL::Luminance16_UNorm;
		info.transfer.format = rg ? OGL::Red : OGL::Luminance;
		if (OpenGLCompat::hasExtension(OpenGLCompat::TextureFloat)) {
			info.transfer.type = OGL::Float32;
			data = fruit.data();
		} else {
			info.transfer.type = OGL::UInt16;
			buffer.resize(sizeof(GLushort)*fruit.size());
			auto p = (GLushort*)buffer.data();
			for (auto v : fruit)
				*p++ = v*_Max<GLushort>();
			data = buffer.data();
		}
	} else {
		size = 8;
		buffer.resize(size*size);
		mp_make_ordered_dither_matrix((uchar*)buffer.data(), size);
		info = OpenGLCompat::textureTransferInfo(OGL::OneComponent);
		data = buffer.data();
	}
	OpenGLTextureBinder<OGL::Target2D>(&texture)->initialize(size, size, info, data);
}

void HQTextureRendererItem::updateTextureGeometry(QSGGeometry *geometry) {
	QRectF vtx, txt{0.0, 0.0, 1.0, 1.0}; getCoords(vtx, txt);
	d->set(geometry->vertexDataAsTexturedPoint2D(), vtx, txt);
}

QSGNode *HQTextureRendererItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
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
	if (m_interpolator->type() != m_newInt) {
		m_interpolator = Interpolator::get(m_newInt);
		m_interpolator->allocate(m_lutInt[0], m_lutInt[1]);
	}
	if (_Change(m_dithering, m_newDithering))
		makeDitheringTexture(m_ditheringTex, m_dithering);
	return d->node;
}

void HQTextureRendererItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	GeometryItem::geometryChanged(newOne, old);
	m_dirtyGeomerty = true;
	update();
}
