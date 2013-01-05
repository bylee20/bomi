#ifndef VIDEOSHADER_HPP
#define VIDEOSHADER_HPP

#include "stdafx.hpp"
#include "videoframe.hpp"
#include "colorproperty.hpp"
#include "pref.hpp"
#include "videorenderer.hpp"

class VideoShader {
public:
	typedef VideoRenderer::Effect Effect;
	typedef VideoRenderer::Effects Effects;
	struct Var {
		Var() {
			setColor(m_color);
			setEffects(m_effects);
		}
		inline const ColorProperty &color() const {return m_color;}
		inline void setColor(const ColorProperty &color) {
			m_color = color;
			brightness = qBound(-1.0, m_color.brightness(), 1.0);
			contrast = qBound(0., m_color.contrast() + 1., 2.);
			updateHS();
		}
		inline int setEffects(Effects effects) {
			m_effects = effects;
			int idx = 0;
			rgb_0 = 0.0;
			rgb_c[0] = rgb_c[1] = rgb_c[2] = 1.0;
			kern_c = kern_d = kern_n = 0.0;
			if (!(effects & VideoRenderer::IgnoreEffect)) {
				if (effects & VideoRenderer::FilterEffects) {
					idx = 1;
					if (effects & VideoRenderer::InvertColor) {
						rgb_0 = 1.0;
						rgb_c[0] = rgb_c[1] = rgb_c[2] = -1.0;
					}
				}
				if (effects & VideoRenderer::KernelEffects) {
					idx = 2;
					const Pref &p = Pref::get();
					if (effects & VideoRenderer::Blur) {
						kern_c += p.blur_kern_c;
						kern_n += p.blur_kern_n;
						kern_d += p.blur_kern_d;
					}
					if (effects & VideoRenderer::Sharpen) {
						kern_c += p.sharpen_kern_c;
						kern_n += p.sharpen_kern_n;
						kern_d += p.sharpen_kern_d;
					}
					const double den = 1.0/(kern_c + kern_n*4.0 + kern_d*4.0);
					kern_c *= den;
					kern_d *= den;
					kern_n *= den;
				}
			}
			updateHS();
			return m_idx = idx;
		}
		Effects effects() const {return m_effects;}
		void setYRange(float min, float max) {y_min = min; y_max = max;}
		int id() const {return m_idx;}
	private:
		void updateHS() {
			double sat_sinhue = 0.0, sat_coshue = 0.0;
			if (!(!(m_effects & VideoRenderer::IgnoreEffect) && (m_effects & VideoRenderer::Grayscale))) {
				const double sat = qBound(0.0, m_color.saturation() + 1.0, 2.0);
				const double hue = qBound(-M_PI, m_color.hue()*M_PI, M_PI);
				sat_sinhue = sat*sin(hue);
				sat_coshue = sat*cos(hue);
			}
			sat_hue[0][0] = sat_hue[1][1] = sat_coshue;
			sat_hue[1][0] = -(sat_hue[0][1] = sat_sinhue);
		}
		float rgb_0, rgb_c[3];
		float kern_d, kern_c, kern_n;
		float y_min = 0.0f, y_max = 1.0f;
		float brightness, contrast, sat_hue[2][2];
		Effects m_effects = 0;
		int m_idx = 0;
		ColorProperty m_color;

		friend class VideoShader;
	};

	VideoShader(const QGLContext *ctx);
	bool bind(const Var &var, const VideoFormat &format) {
		if (format.type == VideoFormat::BGRA || format.type == VideoFormat::RGBA)
			m_currentProgram = m_rgbProgram;
		else
			m_currentProgram = m_programs[var.id()];
		const bool ret = m_currentProgram && m_currentProgram->bind();
		if (ret)
			m_currentProgram->setUniforms(var, format);
		return ret;
	}
	void release() {if (m_currentProgram) m_currentProgram->release();}
	bool link(VideoFormat::Type type);
	bool isLinked() const {return m_currentProgram && m_currentProgram->isLinked();}
	~VideoShader();
private:
	class Program {
		QGLShaderProgram program;
		QGLShader main;
		int loc_rgb_0, loc_rgb_c, loc_kern_d, loc_kern_c, loc_kern_n, loc_y_tan, loc_y_b;
		int loc_brightness, loc_contrast, loc_sat_hue, loc_p1, loc_p2, loc_p3, loc_dxy;
	public:
		Program(const QGLContext *ctx, const char *main)
		: program(ctx), main(QGLShader::Fragment) {
			this->main.compileSourceCode(main);
			program.addShader(&this->main);
		}
		bool link() {
			const bool ret = program.link();
			if (ret) {
				loc_p1 = program.uniformLocation("p1");
				loc_p2 = program.uniformLocation("p2");
				loc_p3 = program.uniformLocation("p3");
				loc_brightness = program.uniformLocation("brightness");
				loc_contrast = program.uniformLocation("contrast");
				loc_sat_hue = program.uniformLocation("sat_hue");
				loc_rgb_c = program.uniformLocation("rgb_c");
				loc_rgb_0 = program.uniformLocation("rgb_0");
				loc_y_tan = program.uniformLocation("y_tan");
				loc_y_b = program.uniformLocation("y_b");
				loc_dxy = program.uniformLocation("dxy");
				loc_kern_c = program.uniformLocation("kern_c");
				loc_kern_d = program.uniformLocation("kern_d");
				loc_kern_n = program.uniformLocation("kern_n");
			}
			return ret;
		}
		void addShader(QGLShader *shader) {program.addShader(shader);}
		bool isLinked() const {return program.isLinked();}
		bool bind() {return program.bind();}
		void release() {return program.release();}
		void setUniforms(const Var &var, const VideoFormat &format) {
			program.setUniformValue(loc_p1, 0);
			program.setUniformValue(loc_p2, 1);
			program.setUniformValue(loc_p3, 2);
			program.setUniformValue(loc_brightness, var.brightness);
			program.setUniformValue(loc_contrast, var.contrast);
			program.setUniformValue(loc_sat_hue, var.sat_hue);
			const float dx = 1.0/(double)format.stride;
			const float dy = 1.0/(double)format.height;
			program.setUniformValue(loc_dxy, dx, dy, -dx, 0.f);

			const bool filter = var.effects() & VideoRenderer::FilterEffects;
			const bool kernel = var.effects() & VideoRenderer::KernelEffects;
			if (filter || kernel) {
				program.setUniformValue(loc_rgb_c, var.rgb_c[0], var.rgb_c[1], var.rgb_c[2]);
				program.setUniformValue(loc_rgb_0, var.rgb_0);
				const float y_tan = 1.0/(var.y_max - var.y_min);
				program.setUniformValue(loc_y_tan, y_tan);
				program.setUniformValue(loc_y_b, (float)-var.y_min*y_tan);
			}
			if (kernel) {
				program.setUniformValue(loc_kern_c, var.kern_c);
				program.setUniformValue(loc_kern_n, var.kern_n);
				program.setUniformValue(loc_kern_d, var.kern_d);
			}
		}
	};


	Program *m_currentProgram = nullptr;
	Program *m_programs[3];
	Program *m_rgbProgram;
	QGLShader m_common, m_filter, m_kernel, m_simple;
	enum TextureType {YV12 = 0, YUY2, NV12, NV21, RGB, TypeMax};
	VideoFormat::Type m_type = VideoFormat::Unknown;
//	QVector<QGLShader*> m_shaders{QVector<QGLShader*>(TypeMax, nullptr)};
	QGLShader m_yuv;
	const char *m_codes[TypeMax];
//	QGLShader *m_currentShader = nullptr;

};

#endif // VIDEOSHADER_HPP
