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
	VideoFrameShader();
	VideoFrameShader(const VideoFrameShader &) = delete;
	VideoFrameShader &operator = (const VideoFrameShader &) = delete;
	~VideoFrameShader();
	void render(const Kernel3x3 &k3x3);
	GLenum target() const { return m_target; }
	void setDeintMethod(DeintMethod method);
	void setEffects(int effects);
	void setColor(const ColorProperty &color);
	bool upload(VideoFrame &frame);
	const QRectF &textureRect() const { return m_coords; }
	void getCoords(double &x1, double &y1, double &x2, double &y2) {
		x1 = m_coords.left(); y1 = m_coords.top();
		x2 = m_coords.right(); y2 = m_coords.bottom();
	}
private:
	void release();
	void updateColorMatrix();
	static bool isKernelEffect(int effect) { return VideoRendererItem::KernelEffects & effect; }
	static bool isColorEffect(int effect) { return VideoRendererItem::ColorEffects & effect; }
	bool hasKernelEffects() const { return isKernelEffect(m_effects); }
	void fillInfo();
	void fillArrays();
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
private:
	struct ShaderInfo {
		QOpenGLShaderProgram program;
		bool rebuild = true, kernel = false;
	};
	void updateShader();
	VideoFrame m_frame;
	ShaderInfo m_shaders[2];
	QOpenGLShaderProgram *m_prog = nullptr;
	ColorProperty m_color;
	mp_csp m_csp; mp_csp_levels m_range;
	GLenum m_target = GL_TEXTURE_2D;
	QMatrix3x3 m_mul_mat;
	QMatrix4x4 m_vMatrix;
	QVector3D m_sub_vec, m_add_vec;
	QVector<GLfloat> m_vCoords, m_vPositions;
	int m_effects = 0;
	DeintMethod m_deint = DeintMethod::None;
	int loc_kern_d, loc_kern_c, loc_kern_n, loc_top_field;
	int loc_sub_vec, loc_add_vec, loc_mul_mat, loc_vMatrix;
	int loc_tex[3] = {-1, -1, -1}, loc_cc[3] = {-1, -1, -1};
	QList<VideoTexture> m_textures;
	QByteArray m_texel;
	bool m_dma = false, m_check = true;
	QRectF m_coords, m_positions;
	QPointF m_chroma = {0.0, 0.0};
#ifdef Q_OS_LINUX
	void *m_vaSurfaceGLX = nullptr;
#endif
};

#endif // VIDEOFRAMESHADER_HPP
