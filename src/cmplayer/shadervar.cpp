#include "shadervar.h"
#include "pref.hpp"

int ShaderVar::setEffects(VideoRendererItem::Effects effects) {
    m_effects = effects;
    int idx = 0;
    rgb_0 = 0.0;
    rgb_c[0] = rgb_c[1] = rgb_c[2] = 1.0;
    kern_c = kern_d = kern_n = 0.0;
    if (!(effects & VideoRendererItem::IgnoreEffect)) {
        if (effects & VideoRendererItem::FilterEffects) {
            idx = 1;
            if (effects & VideoRendererItem::InvertColor) {
                rgb_0 = 1.0;
                rgb_c[0] = rgb_c[1] = rgb_c[2] = -1.0;
            }
        }
        if (effects & VideoRendererItem::KernelEffects) {
            idx = 2;
            const Pref &p = cPref;
            if (effects & VideoRendererItem::Blur) {
                kern_c += p.blur_kern_c;
                kern_n += p.blur_kern_n;
                kern_d += p.blur_kern_d;
            }
            if (effects & VideoRendererItem::Sharpen) {
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
