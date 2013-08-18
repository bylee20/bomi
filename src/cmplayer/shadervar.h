#ifndef SHADERVAR_H
#define SHADERVAR_H

#include "videorendereritem.hpp"
#include "colorproperty.hpp"
#include "global.hpp"
#include "videoformat.hpp"

class ShaderVar {
public:
    ShaderVar() { setColor(m_color); setEffects(m_effects); }
    inline const ColorProperty &color() const {return m_color;}
    inline void setColor(const ColorProperty &color) {
        m_color = color;
        brightness = qBound(-1.0, m_color.brightness(), 1.0);
        contrast = qBound(0., m_color.contrast() + 1., 2.);
        updateHS();
    }
	bool setEffects(VideoRendererItem::Effects effects);
    VideoRendererItem::Effects effects() const {return m_effects;}
	void setLumaRange(const RangeF &luma) {m_luma = luma;}
//    void setYRange(float min, float max) {y_min = min; y_max = max;}
	int id() const {return m_idx;}
	void setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d) {
		m_blur_kern_c = blur_c;
		m_blur_kern_n = blur_n;
		m_blur_kern_d = blur_d;
		m_sharpen_kern_c = sharpen_c;
		m_sharpen_kern_n = sharpen_n;
		m_sharpen_kern_d = sharpen_d;
		setEffects(m_effects);
	}
private:
    void updateHS() {
        double sat_sinhue = 0.0, sat_coshue = 0.0;
        if (!(!(m_effects & VideoRendererItem::IgnoreEffect) && (m_effects & VideoRendererItem::Grayscale))) {
            const double sat = qBound(0.0, m_color.saturation() + 1.0, 2.0);
            const double hue = qBound(-M_PI, m_color.hue()*M_PI, M_PI);
            sat_sinhue = sat*sin(hue);
            sat_coshue = sat*cos(hue);
        }
        sat_hue[0][0] =  (sat_hue[1][1] = sat_coshue);
        sat_hue[1][0] = -(sat_hue[0][1] = sat_sinhue);
    }
    float rgb_0, rgb_c[3];
    float kern_d, kern_c, kern_n;
//    float y_min = 0.0f, y_max = 1.0f;
    float brightness, contrast, sat_hue[2][2];
    VideoRendererItem::Effects m_effects = 0;
	RangeF m_luma = {0.0, 1.0};
	int m_idx = 0;
    ColorProperty m_color;
    friend class VideoRendererItem;
	friend class TextureShader;
	int m_blur_kern_c = 1, m_blur_kern_n = 1, m_blur_kern_d = 1;
	int m_sharpen_kern_c = 1, m_sharpen_kern_n = 1, m_sharpen_kern_d = 1;
};



#endif // SHADERVAR_H
