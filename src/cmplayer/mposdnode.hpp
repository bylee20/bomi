#ifndef MPOSDNODE_HPP
#define MPOSDNODE_HPP

#include "mposdbitmap.hpp"

class MpOsdNode : public QSGGeometryNode {
public:
	MpOsdNode(MpOsdBitmap::Format format);
	~MpOsdNode();
	MpOsdBitmap::Format format() const { return m_format; }
	void draw(const MpOsdBitmap &osd, const QRectF &rect);
protected:
	void initializeBgTexture(const MpOsdBitmap &osd, GLenum internal, GLenum glFormat, GLenum glType);
	void upload(const MpOsdBitmap &osd, int i);
	GLuint bgTexture() const { return m_bgTexture; }
	int bgTextureWidth() const { return m_bgTextureSize.width(); }
	int bgTextureHeight() const { return m_bgTextureSize.height(); }
	virtual void beforeRendering(const MpOsdBitmap &osd) { Q_UNUSED(osd); }
	virtual void afterRendering() {}
private:
	class Material; class Shader;
	void bind(const QOpenGLShaderProgram *prog) {
		m_loc_matrix = prog->uniformLocation("qt_Matrix");
		m_loc_tex_data = prog->uniformLocation("tex_data");
		m_loc_width = prog->uniformLocation("width");
		m_loc_height = prog->uniformLocation("height");
	}
	void render(QOpenGLShaderProgram *prog, const QSGMaterialShader::RenderState &state) {
		if (state.isMatrixDirty())
			prog->setUniformValue(m_loc_matrix, state.combinedMatrix());
		prog->setUniformValue(m_loc_tex_data, 0);
		if (m_fbo) {
			prog->setUniformValue(m_loc_width, (float)m_fbo->width());
			prog->setUniformValue(m_loc_height, (float)m_fbo->height());
			auto f = QOpenGLContext::currentContext()->functions();
			f->glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
		}
	}
	MpOsdBitmap::Format m_format = MpOsdBitmap::Ass;
	GLenum m_srcFactor = GL_SRC_ALPHA, m_glFormat = GL_ALPHA, m_glType = GL_UNSIGNED_BYTE;
	QVector<float> m_positions, m_coordinates;
	QSize m_bgTextureSize = {0, 0};
	GLuint m_bgTexture = GL_NONE;
	QOpenGLFramebufferObject *m_fbo = nullptr;
	int m_loc_tex_data = 0, m_loc_width = 0, m_loc_height = 0, m_loc_matrix = 0;
};

class MpRgbaOsdNode : public MpOsdNode {
public:
	MpRgbaOsdNode(MpOsdBitmap::Format format): MpOsdNode(format) {
		m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
			uniform sampler2D tex;
			void main() {
				gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);
			}
		)");
		m_shader.link();
		loc_tex = m_shader.uniformLocation("tex");
	}
protected:
	virtual void beforeRendering(const MpOsdBitmap &osd) {
		initializeBgTexture(osd, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		for (int i=0; i<osd.count(); ++i)
			upload(osd, i);
		m_shader.bind();
		m_shader.setUniformValue(loc_tex, 0);
	}
	virtual void afterRendering() { m_shader.release(); }
private:
	int loc_tex = -1;
	QOpenGLShaderProgram m_shader;
};

struct MpAssOsdNode : public MpOsdNode {
	MpAssOsdNode(): MpOsdNode(MpOsdBitmap::Ass) {
		m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
			uniform sampler2D tex;
			varying vec4 c;
			void main() {
				vec2 co = vec2(c.a*texture2D(tex, gl_TexCoord[0].xy).a, 0.0);
				gl_FragColor = c*co.xxxy + co.yyyx;
			}
		)");
		m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
			varying vec4 c;
			void main() {
				c = gl_Color;
				gl_TexCoord[0] = gl_MultiTexCoord0;
				gl_Position = ftransform();
			}
		 )");
		m_shader.link();
		loc_tex = m_shader.uniformLocation("tex");
	}
protected:
	void beforeRendering(const MpOsdBitmap &osd) {
		initializeBgTexture(osd, GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE);
		const int num = osd.count();
		if (num > m_colors.size()/(4*4))
			m_colors.resize(4*4*num*1.5);
		auto pr = m_colors.data();
		for (int i=0; i<num; ++i) {
			auto &part = osd.part(i);
			upload(osd, i);
			const quint32 color = part.color();
			const uchar r = (color >> 24) & 0xff;
			const uchar g = (color >> 16) & 0xff;
			const uchar b = (color >>  8) & 0xff;
			const uchar a = 0xff - (color & 0xff);
			*pr++ = r; *pr++ = g; *pr++ = b; *pr++ = a;
			*pr++ = r; *pr++ = g; *pr++ = b; *pr++ = a;
			*pr++ = r; *pr++ = g; *pr++ = b; *pr++ = a;
			*pr++ = r; *pr++ = g; *pr++ = b; *pr++ = a;
		}
		m_shader.bind();
		m_shader.setUniformValue(loc_tex, 0);

		glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_colors.data());
		glEnableClientState(GL_COLOR_ARRAY);
	}
	void afterRendering() {
		glDisableClientState(GL_COLOR_ARRAY);
		m_shader.release();
	}
private:
	QOpenGLShaderProgram m_shader;
	int loc_tex = 0;
	QVector<uchar> m_colors;
};


#endif // MPOSDNODE_HPP
