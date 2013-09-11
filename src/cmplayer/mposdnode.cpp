#include "mposdnode.hpp"

static int MaterialId = 0;
static std::array<QSGMaterialType, 50> MaterialTypes;

class MpOsdNode::Shader : public QSGMaterialShader {
public:
	Shader(MpOsdNode *node): m_node(node) {}
	void updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
		Q_UNUSED(old); Q_UNUSED(newOne);
		m_node->render(program(), state);
	}
	void initialize() {
		QSGMaterialShader::initialize();
		m_node->bind(program());
	}
private:
	virtual const char *const *attributeNames() const override {
		static const char *names[] = {
			"qt_VertexPosition",
			"qt_VertexTexCoord",
			0
		};
		return names;
	}
	virtual const char *vertexShader() const override {
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
	virtual const char *fragmentShader() const override {
		static const char *shader = (R"(
		uniform sampler2D tex_data;
		uniform float width, height;
		varying vec2 qt_TexCoord;
		void main() {
			vec2 size = vec2(width, height);
			vec3 dxy0 = vec3(1.0/width, 1.0/height, 0.0);
			ivec2 pixel = ivec2(qt_TexCoord*size);
			vec2 texel = (vec2(pixel)+vec2(0.5, 0.5))/size;

			vec4 x0y0 = texture2D(tex_data, texel);
			vec4 x1y0 = texture2D(tex_data, texel + dxy0.xz);
			vec4 x0y1 = texture2D(tex_data, texel + dxy0.zy);
			vec4 x1y1 = texture2D(tex_data, texel + dxy0.xy);

			float a = fract(qt_TexCoord.x*width);
			float b = fract(qt_TexCoord.y*height);
			gl_FragColor =  mix(mix(x0y0, x1y0, a), mix(x0y1, x1y1, a), b);
		}
		)");
		return shader;
	}
	int m_loc_matrix = 0, m_loc_tex_data = 0, m_loc_width = 0, m_loc_height = 0;
	MpOsdNode *m_node = nullptr;
	mutable QByteArray m_frag, m_vtx;
};

struct MpOsdNode::Material : public QSGMaterial {
	Material(MpOsdNode *node): m_node(node) { setFlag(Blending); }
	QSGMaterialType *type() const { return &MaterialTypes[m_id]; }
	QSGMaterialShader *createShader() const { return new Shader(m_node); }
private:
	int m_id = ++MaterialId%MaterialTypes.size();
	MpOsdNode *m_node = nullptr;
};

MpOsdNode::MpOsdNode(MpOsdBitmap::Format format): m_format(format)
, m_srcFactor(format & MpOsdBitmap::PaMask ? GL_ONE : GL_SRC_ALPHA) {
	setFlags(OwnsGeometry | OwnsMaterial);
	setMaterial(new Material(this));
	setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
	markDirty(DirtyMaterial | DirtyGeometry);
}

MpOsdNode::~MpOsdNode() {
	delete m_fbo;
	if (m_bgTexture != GL_NONE)
		glDeleteTextures(1, &m_bgTexture);
}

void MpOsdNode::upload(const MpOsdBitmap &osd, int i) {
	auto &part = osd.part(i);
	glBindTexture(GL_TEXTURE_2D, m_bgTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, part.map().x(), part.map().y(), part.strideAsPixel(), part.size().height(), m_glFormat, m_glType, osd.data(i));
	auto pc = m_coordinates.data() + 4*2*i;
	const float tx1 = (double)part.map().x()/(double)bgTextureWidth();
	const float ty1 = (double)part.map().y()/(double)bgTextureHeight();
	const float tx2 = tx1+(double)part.size().width()/(double)bgTextureWidth();
	const float ty2 = ty1+(double)part.size().height()/(double)bgTextureHeight();
	*pc++ = tx1; *pc++ = ty1;
	*pc++ = tx1; *pc++ = ty2;
	*pc++ = tx2; *pc++ = ty2;
	*pc++ = tx2; *pc++ = ty1;

	auto pp = m_positions.data() + 4*2*i;
	const float vx1 = part.display().left();
	const float vy1 = m_fbo->height() - part.display().top();
	const float vx2 = vx1 + part.display().width();
	const float vy2 = vy1 - part.display().height();
	*pp++ = vx1; *pp++ = vy1;
	*pp++ = vx1; *pp++ = vy2;
	*pp++ = vx2; *pp++ = vy2;
	*pp++ = vx2; *pp++ = vy1;
}

void MpOsdNode::initializeBgTexture(const MpOsdBitmap &osd, GLenum internal, GLenum glFormat, GLenum glType) {
	if (m_bgTextureSize.isEmpty() || osd.TextureMapSize().width() > m_bgTextureSize.width() || osd.TextureMapSize().height() > m_bgTextureSize.height()) {
		if (osd.TextureMapSize().width() > m_bgTextureSize.width())
			m_bgTextureSize.setWidth(qMin<int>(_Aligned<4>(osd.TextureMapSize().width()*1.5), MaxTextureSize));
		if (osd.TextureMapSize().height() > m_bgTextureSize.height())
			m_bgTextureSize.setHeight(qMin<int>(_Aligned<4>(osd.TextureMapSize().height()*1.5), MaxTextureSize));
		if (m_bgTexture == GL_NONE)
			glGenTextures(1, &m_bgTexture);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_bgTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, internal, m_bgTextureSize.width(), m_bgTextureSize.height(), 0, m_glFormat = glFormat, m_glType = glType, nullptr);
		_InitTexParam(GL_NEAREST);
	}
}

void MpOsdNode::draw(const MpOsdBitmap &osd, const QRectF &rect) {
	if (!m_fbo || osd.renderSize() != m_fbo->size()) {
		delete m_fbo;
		m_fbo = new QOpenGLFramebufferObject(osd.renderSize(), GL_TEXTURE_2D);
	}
	m_fbo->bind();
	glViewport(0, 0, m_fbo->width(), m_fbo->height());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, m_fbo->width(), m_fbo->height(), 0, -999999, 999999);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const int num = osd.count();
	if (num > m_positions.size()/(4*2)) {
		const int size = num*1.5;
		m_coordinates.resize(4*2*size);
		m_positions.resize(4*2*size);
	}

	beforeRendering(osd);
	glTexCoordPointer(2, GL_FLOAT, 0, m_coordinates.data());
	glVertexPointer(2, GL_FLOAT, 0, m_positions.data());
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bgTexture());
	glEnable(GL_BLEND);
	glBlendFunc(m_srcFactor, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_QUADS, 0, 4*osd.count());
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	afterRendering();
	m_fbo->release();

	auto tp = geometry()->vertexDataAsTexturedPoint2D();
	const QRectF txt(0, 0, 1, 1);
	auto set = [](QSGGeometry::TexturedPoint2D *tp, const QPointF &vtx, const QPointF &txt) {
		tp->set(vtx.x(), vtx.y(), txt.x(), txt.y());
	};
	set(tp, rect.topLeft(), txt.topLeft());
	set(++tp, rect.bottomLeft(), txt.bottomLeft());
	set(++tp, rect.topRight(), txt.topRight());
	set(++tp, rect.bottomRight(), txt.bottomRight());
}
