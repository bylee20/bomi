#include "mposdnode.hpp"

class MpOsdNode::Shader : public QSGMaterialShader {
public:
	Shader(MpOsdNode *node): m_node(node) {}
	void updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
		Q_UNUSED(old); Q_UNUSED(newOne);
		auto prog = program();
		prog->setUniformValue(loc_vMatrix, state.combinedMatrix());
		prog->setUniformValue(loc_tex, 0);
		OpenGLCompat::functions()->glActiveTexture(GL_TEXTURE0);
		m_node->bindTexture();
	}
	void initialize() {
		QSGMaterialShader::initialize();
		auto prog = program();
		loc_vMatrix = prog->uniformLocation("vMatrix");
		loc_tex = prog->uniformLocation("tex");
	}
private:
	virtual const char *const *attributeNames() const override {
		static const char *names[] = { "vPosition", "vCoord", nullptr };
		return names;
	}
	virtual const char *vertexShader() const override {
		static const char *shader = (R"(
			uniform highp mat4 vMatrix;
			attribute highp vec4 vPosition;
			attribute highp vec2 vCoord;
			varying highp vec2 texCoord;
			void main() {
				texCoord = vCoord;
				gl_Position = vMatrix*vPosition;
			}
		)");
		return shader;
	}
	virtual const char *fragmentShader() const override {
		static const char *shader = (R"(
			uniform sampler2D tex;
			varying vec2 texCoord;
			void main() {
				gl_FragColor = texture2D(tex, texCoord);
			}
		)");
		return shader;
	}
	int loc_vMatrix = 0, loc_tex = 0;
	MpOsdNode *m_node = nullptr;
};

class MpOsdNode::Material : public QSGMaterial {
public:
	Material(MpOsdNode *node): m_node(node) { setFlag(Blending); }
	QSGMaterialType *type() const { return &m_type; }
	QSGMaterialShader *createShader() const { return new Shader(m_node); }
private:
	mutable QSGMaterialType m_type;
	MpOsdNode *m_node = nullptr;
};

MpOsdNode::MpOsdNode() {
	setFlags(OwnsGeometry | OwnsMaterial);
	setMaterial(new Material(this));
	setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
	m_atlas.generate();
}

MpOsdNode::~MpOsdNode() {
	delete m_fbo;
	m_atlas.delete_();
}

void MpOsdNode::build(MpOsdBitmap::Format format) {
	if (!_Change(m_format, format))
		return;
	m_shader.removeAllShaders();
	m_srcFactor = (format & MpOsdBitmap::PaMask) ? GL_ONE : GL_SRC_ALPHA;
	m_atlas.width = m_atlas.height = 0;
	m_atlas.format = OpenGLCompat::textureFormat(format & MpOsdBitmap::Rgba ? GL_BGRA : 1);

	QByteArray frag;
	if (m_format == MpOsdBitmap::Ass) {
		frag = R"(
			uniform sampler2D atlas;
			varying vec4 c;
			varying vec2 texCoord;
			void main() {
				vec2 co = vec2(c.a*texture2D(atlas, texCoord).r, 0.0);
				gl_FragColor = c*co.xxxy + co.yyyx;
			}
		)";
	} else {
		frag = R"(
			uniform sampler2D atlas;
			varying vec2 texCoord;
			void main() {
				gl_FragColor = texture2D(atlas, texCoord);
			}
		)";
	}
	m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
	m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
		uniform mat4 vMatrix;
		varying vec4 c;
		varying vec2 texCoord;
		attribute vec4 vPosition;
		attribute vec2 vCoord;
		attribute vec4 vColor;
		void main() {
			c = vColor.abgr;
			texCoord = vCoord;
			gl_Position = vMatrix*vPosition;
		}
	)");
	m_shader.bindAttributeLocation("vCoord", vCoord);
	m_shader.bindAttributeLocation("vPosition", vPosition);
	m_shader.bindAttributeLocation("vColor", vColor);
	m_shader.link();
	loc_atlas = m_shader.uniformLocation("atlas");
	loc_vMatrix = m_shader.uniformLocation("vMatrix");
}

void MpOsdNode::upload(const MpOsdBitmap &osd, int i) {
	auto &part = osd.part(i);
	m_atlas.upload(part.map().x(), part.map().y(), part.strideAsPixel(), part.height(), osd.data(i));
	auto pc = m_vCoords.data() + 4*2*i;
	const float tx1 = (double)part.map().x()/(double)m_atlas.width;
	const float ty1 = (double)part.map().y()/(double)m_atlas.height;
	const float tx2 = tx1+(double)part.size().width()/(double)m_atlas.width;
	const float ty2 = ty1+(double)part.size().height()/(double)m_atlas.height;
	*pc++ = tx1; *pc++ = ty1;
	*pc++ = tx1; *pc++ = ty2;
	*pc++ = tx2; *pc++ = ty2;
	*pc++ = tx2; *pc++ = ty1;

	auto pp = m_vPositions.data() + 4*2*i;
	const float vx1 = part.display().left();
	const float vy1 = part.display().top();
	const float vx2 = vx1 + part.display().width();
	const float vy2 = vy1 + part.display().height();
	*pp++ = vx1; *pp++ = vy1;
	*pp++ = vx1; *pp++ = vy2;
	*pp++ = vx2; *pp++ = vy2;
	*pp++ = vx2; *pp++ = vy1;

	auto pr = m_vColors.data() + 4*i;
	const auto color = part.color();
	*pr++ = color; *pr++ = color; *pr++ = color; *pr++ = color;
}

void MpOsdNode::initializeAtlas(const MpOsdBitmap &osd) {
	static const int max = OpenGLCompat::maximumTextureSize();
	if (osd.sheet().width() > m_atlas.width || osd.sheet().height() > m_atlas.height) {
		if (osd.sheet().width() > m_atlas.width)
			m_atlas.width = qMin<int>(_Aligned<4>(osd.sheet().width()*1.5), max);
		if (osd.sheet().height() > m_atlas.height)
			m_atlas.height = qMin<int>(_Aligned<4>(osd.sheet().height()*1.5), max);
		glEnable(m_atlas.target);
		m_atlas.allocate(GL_NEAREST);
	}
}

void MpOsdNode::draw(const MpOsdBitmap &osd, const QRectF &rect) {
	build(osd.format());
	if (!m_shader.isLinked())
		return;

	if (!m_fbo || osd.renderSize() != m_fbo->size()) {
		delete m_fbo;
		m_fbo = new OpenGLFramebufferObject(osd.renderSize(), GL_TEXTURE_2D);
		m_vMatrix.setToIdentity();
		m_vMatrix.ortho(0, m_fbo->width(), 0, m_fbo->height(), -1, 1);
	}

	m_fbo->bind();
	glViewport(0, 0, m_fbo->width(), m_fbo->height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const int num = osd.count();
	if (num > m_vPositions.size()/(4*2)) {
		const int size = num*1.5;
		m_vCoords.resize(4*2*size);
		m_vPositions.resize(4*2*size);
		m_vColors.resize(4*size);
	}

	initializeAtlas(osd);

	m_shader.bind();
	m_shader.setUniformValue(loc_atlas, 0);
	m_shader.setUniformValue(loc_vMatrix, m_vMatrix);

	for (int i=0; i<osd.count(); ++i)
		upload(osd, i);

	m_shader.enableAttributeArray(vPosition);
	m_shader.enableAttributeArray(vCoord);
	m_shader.enableAttributeArray(vColor);

	m_shader.setAttributeArray(vColor, GL_UNSIGNED_BYTE, m_vColors.data(), 4);
	m_shader.setAttributeArray(vCoord, m_vCoords.data(), 2);
	m_shader.setAttributeArray(vPosition, m_vPositions.data(), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_atlas.id);
	glEnable(GL_BLEND);
	glBlendFunc(m_srcFactor, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_QUADS, 0, 4*num);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_shader.disableAttributeArray(vCoord);
	m_shader.disableAttributeArray(vPosition);
	m_shader.disableAttributeArray(vColor);

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
