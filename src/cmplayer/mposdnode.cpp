#include "mposdnode.hpp"

static int MaterialId = 0;
static std::array<QSGMaterialType, 50> MaterialTypes;

class MpOsdNode::Shader : public QSGMaterialShader {
public:
	Shader(MpOsdNode *node): m_node(node) {}
	void updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
		Q_UNUSED(old); Q_UNUSED(newOne); m_node->render(program(), state);
	}
	void initialize() { QSGMaterialShader::initialize(); m_node->bind(program()); }
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

class MpOsdNode::Material : public QSGMaterial {
public:
	Material(MpOsdNode *node): m_node(node) { setFlag(Blending); }
	QSGMaterialType *type() const { return &MaterialTypes[m_id]; }
	QSGMaterialShader *createShader() const { return new Shader(m_node); }
private:
	int m_id = ++MaterialId%MaterialTypes.size();
	MpOsdNode *m_node = nullptr;
};

MpOsdNode::MpOsdNode(MpOsdBitmap::Format format, const char *sheet)
: m_format(format), m_sheetName(QString::fromLatin1(sheet))
, m_srcFactor(format & MpOsdBitmap::PaMask ? GL_ONE : GL_SRC_ALPHA) {
	setFlags(OwnsGeometry | OwnsMaterial);
	setMaterial(new Material(this));
	setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
	markDirty(DirtyMaterial | DirtyGeometry);
	glGenTextures(1, &m_sheet.id);
	m_sheet.format = OpenGLCompat::textureFormat(format & MpOsdBitmap::Rgba ? GL_BGRA : 1);
}

MpOsdNode::~MpOsdNode() {
	delete m_fbo;
	glDeleteTextures(1, &m_sheet.id);
}

void MpOsdNode::link(const QByteArray &fragment, const QByteArray &vertex) {
	if (!fragment.isEmpty())
		m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment.data());
	if (!vertex.isEmpty())
		m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex.data());
	m_shader.link();
	loc_sheet = m_shader.uniformLocation(m_sheetName);
}

void MpOsdNode::bind(const QOpenGLShaderProgram *prog) {
	loc_matrix = prog->uniformLocation("qt_Matrix");
	loc_tex_data = prog->uniformLocation("tex_data");
	loc_width = prog->uniformLocation("width");
	loc_height = prog->uniformLocation("height");
}

void MpOsdNode::render(QOpenGLShaderProgram *prog, const QSGMaterialShader::RenderState &state) {
	if (state.isMatrixDirty())
		prog->setUniformValue(loc_matrix, state.combinedMatrix());
	prog->setUniformValue(loc_tex_data, 0);
	if (m_fbo) {
		prog->setUniformValue(loc_width, (float)m_fbo->width());
		prog->setUniformValue(loc_height, (float)m_fbo->height());
		auto f = QOpenGLContext::currentContext()->functions();
		f->glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
	}
}

void MpOsdNode::upload(const MpOsdBitmap &osd, int i) {
	auto &part = osd.part(i);
	m_sheet.upload(part.map().x(), part.map().y(), part.strideAsPixel(), part.height(), osd.data(i));
	auto pc = m_coordinates.data() + 4*2*i;
	const float tx1 = (double)part.map().x()/(double)m_sheet.width;
	const float ty1 = (double)part.map().y()/(double)m_sheet.height;
	const float tx2 = tx1+(double)part.size().width()/(double)m_sheet.width;
	const float ty2 = ty1+(double)part.size().height()/(double)m_sheet.height;
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

void MpOsdNode::initializeSheetTexture(const MpOsdBitmap &osd) {
	static const int max = OpenGLCompat::maximumTextureSize();
	if (osd.sheet().width() > m_sheet.width || osd.sheet().height() > m_sheet.height) {
		if (osd.sheet().width() > m_sheet.width)
			m_sheet.width = qMin<int>(_Aligned<4>(osd.sheet().width()*1.5), max);
		if (osd.sheet().height() > m_sheet.height)
			m_sheet.height = qMin<int>(_Aligned<4>(osd.sheet().height()*1.5), max);
		glEnable(m_sheet.target);
		m_sheet.allocate(GL_NEAREST);
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

	initializeSheetTexture(osd);

	m_shader.bind();
	m_shader.setUniformValue(loc_sheet, 0);

	beforeRendering(osd);
	glTexCoordPointer(2, GL_FLOAT, 0, m_coordinates.data());
	glVertexPointer(2, GL_FLOAT, 0, m_positions.data());
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_sheet.id);
	glEnable(GL_BLEND);
	glBlendFunc(m_srcFactor, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_QUADS, 0, 4*osd.count());
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	afterRendering();
	m_shader.release();
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

/***************************************************************************************/

MpRgbaOsdNode::MpRgbaOsdNode(MpOsdBitmap::Format format): MpOsdNode(format) {
	link(R"(
		uniform sampler2D sheet;
		void main() {
			gl_FragColor = texture2D(sheet, gl_TexCoord[0].xy);
		}
	)");
}

void MpRgbaOsdNode::beforeRendering(const MpOsdBitmap &osd) {
	for (int i=0; i<osd.count(); ++i)
		upload(osd, i);
}

MpAssOsdNode::MpAssOsdNode(): MpOsdNode(MpOsdBitmap::Ass) {
	link(R"(
		uniform sampler2D sheet;
		varying vec4 c;
		void main() {
			vec2 co = vec2(c.a*texture2D(sheet, gl_TexCoord[0].xy).r, 0.0);
			gl_FragColor = c*co.xxxy + co.yyyx;
		}
	)", R"(
		varying vec4 c;
		void main() {
			c = gl_Color.abgr;
			gl_TexCoord[0] = gl_MultiTexCoord0;
			gl_Position = ftransform();
		}
	)");
}

void MpAssOsdNode::beforeRendering(const MpOsdBitmap &osd) {
	const int num = osd.count();
	if (num > m_colors.size()/4)
		m_colors.resize(4*num*1.5);
	quint32 *pr = m_colors.data();
	for (int i=0; i<num; ++i) {
		auto &part = osd.part(i);
		upload(osd, i);
		const quint32 color = part.color();
		*pr++ = color; *pr++ = color; *pr++ = color; *pr++ = color;
	}
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_colors.data());
	glEnableClientState(GL_COLOR_ARRAY);
}
