#include "shadervar.h"
#include "pref.hpp"
#include "videoformat.hpp"

bool ShaderVar::setEffects(VideoRendererItem::Effects effects) {
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
	return _Change(m_idx, idx);
}

static const char YCbCr[] = (R"(
	uniform float brightness, contrast;
	uniform mat2 sat_hue;
	uniform vec3 rgb_c;
	uniform float rgb_0;
	uniform float y_tan, y_b;
	uniform vec4 dxy;
	uniform float kern_c, kern_n, kern_d;
	uniform sampler2D p1, p2, p3;
	uniform vec2 sc;
	varying highp vec2 qt_TexCoord;

	vec3 get_yuv(const vec2 coord);
	void convert(inout vec3 yuv) {
		const vec3 yuv_0 = vec3(0.0625, 0.5, 0.5);

		yuv -= yuv_0;

		yuv.yz *= sat_hue;
		yuv *= contrast;
		yuv.x += brightness;

		const mat3 coef = mat3(
			1.16438356,  0.0,          1.59602679,
			1.16438356, -0.391762290, -0.812967647,
			1.16438356,  2.01723214,   0.0
		);
		yuv *= coef;
	}

	void adjust_rgb(inout vec3 rgb) {
		rgb *= rgb_c;
		rgb += rgb_0;
	}

	void renormalize_y(inout float y) {
		y = y_tan*y + y_b;
	}

	void apply_filter_convert(inout vec3 yuv) {
		renormalize_y(yuv.x);
		convert(yuv);
		adjust_rgb(yuv);
	}

	vec3 get_yuv_kernel_applied(const in vec2 coord) {
		// dxy.zy   dxy.wy   dxy.xy
		// dxy.zw     0      dxy.xw
		//-dxy.xy  -dxy.wy  -dxy.zy
		vec3 c = get_yuv(coord)*kern_c;
		c += (get_yuv(coord + dxy.wy)+get_yuv(coord + dxy.zw)+get_yuv(coord + dxy.xw)+get_yuv(coord - dxy.wy))*kern_n;
		c += (get_yuv(coord + dxy.zy)+get_yuv(coord + dxy.xy)+get_yuv(coord - dxy.xy)+get_yuv(coord - dxy.zy))*kern_d;
		return c;
	}
)");

static const char I420[] = (R"(
	vec3 get_yuv(const vec2 coord) {
		vec3 yuv;
		yuv.x = texture2D(p1, coord).x;
		yuv.y = texture2D(p2, coord*vec2(sc.x, 1.0)).x;
		yuv.z = texture2D(p3, coord*vec2(sc.y, 1.0)).x;
		return yuv;
	}
)");

static const char NV12[] = (R"(
	vec3 get_yuv(const vec2 coord) {
		vec3 yuv;
		yuv.x = texture2D(p1, coord).x;
		yuv.yz = texture2D(p2, coord*vec2(sc.x, 1.0)).xw;
		return yuv;
	}
)");

static const char NV21[] = (R"(
	vec3 get_yuv(const vec2 coord) {
		vec3 yuv;
		yuv.x = texture2D(p1, coord).x;
		yuv.yz = texture2D(p2, coord*vec2(sc.x, 1.0)).wx;
		return yuv;
	}
)");

static const char YUY2[] = (R"(
	vec3 get_yuv(const vec2 coord) {
		vec3 yuv;
		yuv.x = texture2D(p1, coord).x;
		yuv.yz = texture2D(p2, coord).yw;
		return yuv;
	}
)");

static const char UYVY[] = (R"(
	vec3 get_yuv(const vec2 coord) {
		vec3 yuv;
		yuv.x = texture2D(p1, coord).a;
		yuv.yz = texture2D(p2, coord).zx;
		return yuv;
	}
)");

static const char Rgb[] = (R"(
	uniform sampler2D p1;
	varying highp vec2 qt_TexCoord;
	void main() {
		gl_FragColor = texture2D(p1, qt_TexCoord);
	}
)");

static char SimpleMain[] = (R"(
	void main() {
		vec3 c = get_yuv(qt_TexCoord);
		convert(c);
		gl_FragColor.xyz = c;
		gl_FragColor.w = 1.0;
	}
)");

static char FilterMain[] = (R"(
void main() {
	vec3 c = get_yuv(qt_TexCoord);
	apply_filter_convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
}
)");

static char KernelMain[] = (R"(
	void main() {
	vec3 c = get_yuv_kernel_applied(qt_TexCoord);
	apply_filter_convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
	}
)");

QByteArray ShaderVar::fragment(int frameType) const {
	QByteArray shader;
	if (VideoFormat::isYCbCr(frameType)) {
		shader = YCbCr;
		switch (frameType) {
		case VideoFormat::I420:
			shader.append(I420);
			break;
		case VideoFormat::NV12:
			shader.append(NV12);
			break;
		case VideoFormat::NV21:
			shader.append(NV21);
			break;
		case VideoFormat::YUY2:
			shader.append(YUY2);
			break;
		case VideoFormat::UYVY:
			shader.append(UYVY);
			break;
		default:
			break;
		}
		if (m_effects & VideoRendererItem::FilterEffects)
			shader.append(FilterMain);
		else if (m_effects & VideoRendererItem::KernelEffects)
			shader.append(KernelMain);
		else
			shader.append(SimpleMain);
	} else {
		shader = Rgb;
	}
	return shader;
}
