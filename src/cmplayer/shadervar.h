#ifndef SHADERVAR_H
#define SHADERVAR_H

#include "videorendereritem.hpp"
#include "colorproperty.hpp"
#include "global.hpp"

struct ShaderVar {
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
	QByteArray fragment(int frameType) const;
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
};


#endif // SHADERVAR_H
