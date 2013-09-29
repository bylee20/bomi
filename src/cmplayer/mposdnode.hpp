#ifndef MPOSDNODE_HPP
#define MPOSDNODE_HPP

#include "mposdbitmap.hpp"
#include "openglcompat.hpp"

class MpOsdNode : public QSGGeometryNode {
public:
	enum {vPosition = 0, vCoord, vColor};
	MpOsdNode();
	~MpOsdNode();
	MpOsdBitmap::Format format() const { return m_format; }
	void draw(const MpOsdBitmap &osd, const QRectF &rect);
protected:
	void initializeAtlas(const MpOsdBitmap &osd);
	void upload(const MpOsdBitmap &osd, int i);
	virtual void beforeRendering(const MpOsdBitmap &osd) { Q_UNUSED(osd); }
	virtual void afterRendering() {}
	QOpenGLShaderProgram &program() { return m_shader; }
	QVector<quint32> m_vColors;
private:
	void build(MpOsdBitmap::Format format);
	class Material; class Shader;
	void bindTexture() { if (m_fbo) m_fbo->texture().bind(); }
	MpOsdBitmap::Format m_format = MpOsdBitmap::Ass;
	GLenum m_srcFactor = GL_SRC_ALPHA;
	int loc_atlas = 0, loc_vMatrix = 0;
	QVector<float> m_vPositions, m_vCoords;
	OpenGLFramebufferObject *m_fbo = nullptr;
	QOpenGLShaderProgram m_shader;
	OpenGLTexture m_atlas;
	QMatrix4x4 m_vMatrix;
};

#endif // MPOSDNODE_HPP
