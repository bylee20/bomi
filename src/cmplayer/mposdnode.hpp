#ifndef MPOSDNODE_HPP
#define MPOSDNODE_HPP

#include "mposdbitmap.hpp"
#include "openglcompat.hpp"

class MpOsdNode : public QSGGeometryNode {
public:
	MpOsdNode(MpOsdBitmap::Format format, const char *sheet = "sheet");
	~MpOsdNode();
	MpOsdBitmap::Format format() const { return m_format; }
	void draw(const MpOsdBitmap &osd, const QRectF &rect);
protected:
	void initializeSheetTexture(const MpOsdBitmap &osd);
	void upload(const MpOsdBitmap &osd, int i);
//	int bgTextureWidth() const { return m_tileSize.width(); }
//	int bgTextureHeight() const { return m_tileSize.height(); }
	virtual void beforeRendering(const MpOsdBitmap &osd) { Q_UNUSED(osd); }
	virtual void afterRendering() {}
	void link(const QByteArray &fragment, const QByteArray &vertex = QByteArray());
private:
	class Material; class Shader;
	void bind(const QOpenGLShaderProgram *prog);
	void render(QOpenGLShaderProgram *prog, const QSGMaterialShader::RenderState &state);
	MpOsdBitmap::Format m_format = MpOsdBitmap::Ass;
	QString m_sheetName;
	GLenum m_srcFactor = GL_SRC_ALPHA;
	int loc_tex_data = 0, loc_width = 0, loc_height = 0, loc_matrix = 0, loc_sheet = 0;
	QVector<float> m_positions, m_coordinates;
	QOpenGLFramebufferObject *m_fbo = nullptr;
	QOpenGLShaderProgram m_shader;
	OpenGLTexture m_sheet;
};

class MpRgbaOsdNode : public MpOsdNode {
public:
	MpRgbaOsdNode(MpOsdBitmap::Format format);
protected:
	virtual void beforeRendering(const MpOsdBitmap &osd) override;
};

struct MpAssOsdNode : public MpOsdNode {
	MpAssOsdNode();
protected:
	void beforeRendering(const MpOsdBitmap &osd) override;
	void afterRendering() { glDisableClientState(GL_COLOR_ARRAY); }
private:
	QVector<quint32> m_colors;
};


#endif // MPOSDNODE_HPP
