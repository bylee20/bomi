//#include "texturenode.hpp"
//#include "openglcompat.hpp"

//class TextureNode::Material : public QSGMaterial {
//public:
//	Material(TextureNode *node): m_node(node) { setFlag(Blending, !m_node->isOpaque()); }
//	QSGMaterialType *type() const { return m_node->type(); }
//	QSGMaterialShader *createShader() const { return m_node->createShader(); }
//private:
//	TextureNode *m_node = nullptr;
//};

//TextureNode::TextureNode(bool opaque): m_opaque(opaque) {
//	setFlags(OwnsGeometry | OwnsMaterial);
//	setMaterial(new Material(this));
//	setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
//}

//TextureNode::~TextureNode() { }

//QSGMaterialShader *TextureNode::createShader() {
//	return new TextureNodeShader(this);
//}

//QSGMaterialType *TextureNode::type() const {
//	return &m_type;
//}

//void TextureNode::updateGeometry(const QRectF &vertices, const QRectF &texCoords) {
//	auto tp = geometry()->vertexDataAsTexturedPoint2D();
//	auto set = [](QSGGeometry::TexturedPoint2D *tp, const QPointF &vtx, const QPointF &txt) {
//		tp->set(vtx.x(), vtx.y(), txt.x(), txt.y());
//	};
//	set(tp, vertices.topLeft(), texCoords.topLeft());
//	set(++tp, vertices.bottomLeft(), texCoords.bottomLeft());
//	set(++tp, vertices.topRight(), texCoords.topRight());
//	set(++tp, vertices.bottomRight(), texCoords.bottomRight());
//}

//void TextureNode::updateGeometry(const QPointF &vtxTL, const QPointF &vtxBR, const QPointF &txtTL, const QPointF &txtBR) {
//	updateGeometry({vtxTL, vtxBR}, {txtTL, txtBR});
//}

///*******************************************************/

//void TextureNodeShader::bind(QOpenGLShaderProgram *prog) {
//	prog->setUniformValue(loc_tex, 0);
//	OpenGLCompat::functions()->glActiveTexture(GL_TEXTURE0);
//	m_node->bindTexture();
//}

//void TextureNodeShader::link(QOpenGLShaderProgram *prog) {
//	loc_tex = prog->uniformLocation("tex");
//}

//const char *const *TextureNodeShader::attributeNames() const {
//	static const char *names[] = { "vPosition", "vCoord", nullptr };
//	return names;
//}

//const char *TextureNodeShader::vertexShader() const {
//	static const char *shader = (R"(
//		uniform highp mat4 vMatrix;
//		attribute highp vec4 vPosition;
//		attribute highp vec2 vCoord;
//		varying highp vec2 texCoord;
//		void main() {
//			texCoord = vCoord;
//			gl_Position = vMatrix*vPosition;
//		}
//	)");
//	return shader;
//}

//const char *TextureNodeShader::fragmentShader() const {
//	static const char *shader = (R"(
//		uniform sampler2D tex;
//		varying vec2 texCoord;
//		void main() {
//			gl_FragColor = texture2D(tex, texCoord);
//		}
//	)");
//	return shader;
//}

//void TextureNodeShader::updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
//	Q_UNUSED(old); Q_UNUSED(newOne);
//	program()->setUniformValue(loc_vMatrix, state.combinedMatrix());
//	bind(program());
//	OpenGLCompat::functions()->glActiveTexture(GL_TEXTURE0);
//}

//void TextureNodeShader::initialize() {
//	QSGMaterialShader::initialize();
//	loc_vMatrix = program()->uniformLocation("vMatrix");
//	link(program());
//}
