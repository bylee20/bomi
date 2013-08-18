#ifndef TEXTURESHADER_HPP
#define TEXTURESHADER_HPP

#include "videoformat.hpp"
#include "videoframe.hpp"

class VideoFrame;
class ShaderVar;

struct TextureInfo {
	GLuint id = GL_NONE; int plane = 0, width = 0, height = 0; GLenum target = GL_TEXTURE_2D, format = GL_NONE, type = GL_UNSIGNED_BYTE, internal = GL_NONE;
};

struct TextureUploader {
	virtual ~TextureUploader() {}
	virtual void initialize(const TextureInfo &info) {
		glBindTexture(info.target, info.id);
		glTexParameterf(info.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(info.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(info.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(info.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(info.target, 0, info.internal, info.width, info.height, 0, info.format, info.type, nullptr);
	}
	virtual void upload(const TextureInfo &info, const VideoFrame &frame) {
		glBindTexture(info.target, info.id);
		glTexSubImage2D(info.target, 0, 0, 0, info.width, info.height, info.format, info.type, frame.data(info.plane));
	}
	virtual QImage toImage(const VideoFrame &frame) const {
		return frame.toImage();
	}
};

class TextureShader {
public:
	TextureShader(const VideoFormat &format, GLenum target = GL_TEXTURE_2D);
	virtual ~TextureShader() { delete m_uploader; }
	static TextureShader *create(const VideoFormat &format);
	void initialize(GLuint *textures);
	GLuint *textures() const {return m_textures;}
	const VideoFormat &format() const {return m_format;}
	QByteArray fragment(const ShaderVar &var) const;
	void getCoords(double &x1, double &y1, double &x2, double &y2) const { m_rect.getCoords(&x1, &y1, &x2, &y2); }
	void link(QOpenGLShaderProgram *program);
	void render(QOpenGLShaderProgram *program, const ShaderVar &var);
	virtual void upload(const VideoFrame &frame);
	QImage toImage(const VideoFrame &frame) const {return m_uploader->toImage(frame);}
protected:
	virtual QByteArray getMain(const ShaderVar &var) const = 0;
	float &sc(int idx) {return m_strideCorrection[idx];}
	float sc(int idx) const {return m_strideCorrection[idx];}
	void addTexInfo(int plane, int width, int height, GLenum format, GLenum type = GL_UNSIGNED_BYTE, GLenum internal = GL_NONE) {
		TextureInfo info;
		info.plane = plane; info.width = width; info.height = height;			info.target = m_target;
		info.format = format; info.type = type; info.internal = (internal == GL_NONE) ? format : internal;
		m_info.append(info);
	}
	void addRgbInfo(int plane, int width, int height, GLenum format, GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV) {
		addTexInfo(plane, width, height, format, type, 4);
	}
private:
//	void initTex(int idx, int width, int height, GLenum format, GLenum type = GL_UNSIGNED_BYTE, GLenum internal = GL_NONE) {
//		glBindTexture(m_target, m_textures[idx]);
//		glTexParameterf(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameterf(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameterf(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameterf(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		if (internal == GL_NONE)
//			internal = format;
//		glTexImage2D(m_target, 0, internal, width, height, 0, format, type, nullptr);
//	}
	void setUploader(TextureUploader *uploader) {if (m_uploader) delete m_uploader; m_uploader = uploader;}
	GLuint *m_textures = nullptr;
	VideoFormat m_format;
	QByteArray m_variables;
	float m_strideCorrection[3] = {1.0, 1.0, 1.0};
	GLenum m_target = GL_TEXTURE_2D;
//	QPointF m_strideCorrection = {1.0, 1.0}; // texel correction for second and third planar
	//// occasionally, the stride does not match exactly, e.g, for I420, stride[1]*2 != stride[0]
	int loc_rgb_0, loc_rgb_c, loc_kern_d, loc_kern_c, loc_kern_n, loc_y_tan, loc_y_b;
	int loc_brightness, loc_contrast, loc_sat_hue, loc_dxy, loc_p1, loc_p2, loc_p3;
	int loc_sc;
	friend class VdaShader;
	QRectF m_rect = {0, 0, 1, 1};
	QVector<TextureInfo> m_info;
	TextureUploader *m_uploader = nullptr;
};

#endif // TEXTURESHADER_HPP
