#ifndef VIDEOFRAMESHADER_HPP
#define VIDEOFRAMESHADER_HPP

#include "stdafx.hpp"
#include "openglcompat.hpp"
#include "videoformat.hpp"
#include "colorproperty.hpp"
#include "videorendereritem.hpp"
#include "deintinfo.hpp"
#include "videoframe.hpp"

class VideoFrameShader {
	enum {vPosition, vCoord};
public:
	VideoFrameShader(const VideoFormat &format, const ColorProperty &color, int effects, const DeintInfo &deint);
	~VideoFrameShader();
	void build();
	void render(/*QOpenGLShaderProgram *prog, const QMatrix4x4 &mat, */const Kernel3x3 &k3x3);
	GLenum target() const { return m_target; }
	void setDeintInfo(const DeintInfo &deint);
	void setEffects(int effects);
	void setColor(const ColorProperty &color);
	void upload(const VideoFrame &frame);
	int field() const { return m_field; }
	bool needsToBuild() const { return m_rebuild; }
	QByteArray fragment() const { return m_fragCode; }
	QByteArray vertex() const { return m_vertexCode; }
	const char *const *attributes() const;
	void link(QOpenGLShaderProgram *prog);
	const QRectF &textureRect() const { return m_coords; }
	void getCoords(double &x1, double &y1, double &x2, double &y2) {
		x1 = m_coords.left(); y1 = m_coords.top();
		x2 = m_coords.right(); y2 = m_coords.bottom();
	}
private:
	void updateColorMatrix();
	static bool isKernelEffect(int effect) { return VideoRendererItem::KernelEffects & effect; }
	bool hasKernelEffects() const { return isKernelEffect(m_effects); }
	void fillInfo();
	void fillArrays();
	bool tryPixmap();
	static QVector<GLfloat> makeArray(const QPointF &p1, const QPointF &p2) {
		QVector<GLfloat> vec(4*2);
		auto p = vec.data();
		*p++ = p1.x(); *p++ = p1.y();
		*p++ = p1.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p1.y();
		return vec;
	}
	void updateTexCoords();
	void updateVertexPositions() {
		m_vPositions = makeArray({0.0, 0.0}, QPointF(m_format.width(), m_format.height()));
	}
private:
	VideoFormat m_format;
	QOpenGLShaderProgram m_shader;
	ColorProperty m_color;
	mp_csp m_csp;
	mp_csp_levels m_range;
	GLenum m_target = GL_TEXTURE_2D;
	QMatrix3x3 m_mul_mat;
	QMatrix4x4 m_vMatrix;
	QVector3D m_sub_vec, m_add_vec;
	QVector<GLfloat> m_vCoords, m_vPositions;
	int m_effects = 0, m_field = 0;
	DeintInfo m_deint;
	int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field, loc_deint;
	int loc_sub_vec, loc_add_vec, loc_mul_mat, loc_vMatrix;
	int loc_tex[3] = {-1, -1, -1}, loc_cc[3] = {-1, -1, -1};
	QList<VideoTexture2> m_textures;
	QByteArray m_texel, m_fragCode, m_vertexCode;
	void *m_vaSurfaceGLX = nullptr;
	bool m_rebuild = true, m_flipped = false;
	QRectF m_coords, m_positions;
	VideoFrame m_buffer;
};

#endif // VIDEOFRAMESHADER_HPP
