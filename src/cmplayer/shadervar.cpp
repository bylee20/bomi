#include "shadervar.h"

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
			if (effects & VideoRendererItem::Blur) {
				kern_c += m_blur_kern_c;
				kern_n += m_blur_kern_n;
				kern_d += m_blur_kern_d;
			}
			if (effects & VideoRendererItem::Sharpen) {
				kern_c += m_sharpen_kern_c;
				kern_n += m_sharpen_kern_n;
				kern_d += m_sharpen_kern_d;
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

//static const char YCbCr[] = (R"(
//	uniform float brightness, contrast;
//	uniform mat2 sat_hue;
//	uniform vec3 rgb_c;
//	uniform float rgb_0;
//	uniform float y_tan, y_b;
//	uniform vec4 dxy;
//	uniform float kern_c, kern_n, kern_d;
//	uniform sampler2D p1, p2, p3;
//	uniform vec2 sc;
//	varying highp vec2 qt_TexCoord;

//	vec3 get_yuv(const vec2 coord);
//	void convert(inout vec3 yuv) {
//		const vec3 yuv_0 = vec3(0.0625, 0.5, 0.5);

//		yuv -= yuv_0;

//		yuv.yz *= sat_hue;
//		yuv *= contrast;
//		yuv.x += brightness;

//		const mat3 coef = mat3(
//			1.16438356,  0.0,          1.59602679,
//			1.16438356, -0.391762290, -0.812967647,
//			1.16438356,  2.01723214,   0.0
//		);
//		yuv *= coef;
//	}

//	void adjust_rgb(inout vec3 rgb) {
//		rgb *= rgb_c;
//		rgb += rgb_0;
//	}

//	void renormalize_y(inout float y) {
//		y = y_tan*y + y_b;
//	}

//	void apply_filter_convert(inout vec3 yuv) {
//		renormalize_y(yuv.x);
//		convert(yuv);
//		adjust_rgb(yuv);
//	}

//	vec3 get_yuv_kernel_applied(const in vec2 coord) {
//		// dxy.zy   dxy.wy   dxy.xy
//		// dxy.zw     0      dxy.xw
//		//-dxy.xy  -dxy.wy  -dxy.zy
//		vec3 c = get_yuv(coord)*kern_c;
//		c += (get_yuv(coord + dxy.wy)+get_yuv(coord + dxy.zw)+get_yuv(coord + dxy.xw)+get_yuv(coord - dxy.wy))*kern_n;
//		c += (get_yuv(coord + dxy.zy)+get_yuv(coord + dxy.xy)+get_yuv(coord - dxy.xy)+get_yuv(coord - dxy.zy))*kern_d;
//		return c;
//	}
//)");

//static const char I420[] = (R"(
//	vec3 get_yuv(const vec2 coord) {
//		vec3 yuv;
//		yuv.x = texture2D(p1, coord).x;
//		yuv.y = texture2D(p2, coord*vec2(sc.x, 1.0)).x;
//		yuv.z = texture2D(p3, coord*vec2(sc.y, 1.0)).x;
//		return yuv;
//	}
//)");




//static char SimpleMain[] = (R"(
//	void main() {
//		vec3 c = get_yuv(qt_TexCoord);
//		convert(c);
//		gl_FragColor.xyz = c;
//		gl_FragColor.w = 1.0;
//	}
//)");

//static char FilterMain[] = (R"(
//void main() {
//	vec3 c = get_yuv(qt_TexCoord);
//	apply_filter_convert(c);
//	gl_FragColor.xyz = c;
//	gl_FragColor.w = 1.0;
//}
//)");

//static char KernelMain[] = (R"(
//void main() {
//	vec3 c = get_yuv_kernel_applied(qt_TexCoord);
//	apply_filter_convert(c);
//	gl_FragColor.xyz = c;
//	gl_FragColor.w = 1.0;
//}
//)");



//static char APGL[] = (R"(
//uniform float brightness, contrast;
//uniform mat2 sat_hue;
//uniform vec3 rgb_c;
//uniform float rgb_0;
//uniform float y_tan, y_b;
//uniform vec4 dxy;
//uniform float kern_c, kern_n, kern_d;
//uniform sampler2DRect p1, p2, p3;
//uniform vec2 sc;
//varying highp vec2 qt_TexCoord;
//  void main() {
//	float nx,ny,r,g,b,y,u,v;
//	vec4 txl,ux,vx;
//	nx=qt_TexCoord.x;
//	ny=qt_TexCoord.y;


////	y=texture2DRect(p1, vec2(nx,ny)).x;

////	if (mod(floor(nx), 2.0) < 0.5) {
////		u = texture2DRect(p1, vec2(nx,ny)).a;
////		v=texture2DRect(p1, vec2(nx+1.0,ny)).a;
////	} else {
////		u = texture2DRect(p1, vec2(nx-1.0,ny)).a;
////		v = texture2DRect(p1, vec2(nx,ny)).a;
////	}
////	else
////		y=texture2DRect(p2, vec2(nx/2.0,ny)).z;
////	u=texture2DRect(p2, vec2(nx/2.0,ny)).y;


//	y=texture2DRect(p2,vec2(nx/2.0,ny)).x;
//	u=texture2DRect(p2,vec2(nx/2.0,ny)).y;
//	v=texture2DRect(p2,vec2(nx/2.0,ny)).w;
//	y=1.1643*(y-0.0625);
//	u=u-0.5;
//	v=v-0.5;

//	r=y+1.5958*v;
//	g=y-0.39173*u-0.81290*v;
//	b=y+2.017*u;

////	gl_FragColor=vec4(r,g,b,1.0);
////	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);

////	gl_FragColor.w = 1.0;
//	gl_FragColor.xyz = texture2DRect(p1, qt_TexCoord).zzz;
//	gl_FragColor.w = 1.0;
//  }

//)");


//static char APGL[] = (R"(
//uniform float brightness, contrast;
//uniform mat2 sat_hue;
//uniform vec3 rgb_c;
//uniform float rgb_0;
//uniform float y_tan, y_b;
//uniform vec4 dxy;
//uniform float kern_c, kern_n, kern_d;
//uniform sampler2DRect p1, p2, p3;
//uniform vec2 sc;
//varying highp vec2 qt_TexCoord;
//  void main() {
//	float nx,ny,r,g,b,y,u,v;
//	vec4 txl,ux,vx;
//	nx=qt_TexCoord.x;
////	ny=576.0-qt_TexCoord.y;
//	ny=qt_TexCoord.y;
//	y=texture2DRect(p1,vec2(nx,ny)).r;
//	u=texture2DRect(p2,vec2(nx/2.0,ny/2.0)).r;
//	v=texture2DRect(p3,vec2(nx/2.0,ny/2.0)).r;
//	y=1.1643*(y-0.0625);
//	u=u-0.5;
//	v=v-0.5;

//	r=y+1.5958*v;
//	g=y-0.39173*u-0.81290*v;
//	b=y+2.017*u;

//	gl_FragColor=vec4(r,g,b,1.0);
////	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
//	gl_FragColor=texture2DRect(p3, qt_TexCoord);
//  }

//)");



//QByteArray ShaderVar::fragment(int frameType) const {
//////	return APGL;
////	QByteArray shader;
////	if (VideoFormat::APGL == frameType) {
////		shader = Rgb;
////	} else if (VideoFormat::isYCbCr(frameType)) {
////		shader = YCbCr;
////		switch (frameType) {
////		case VideoFormat::I420:
////			shader.append(I420);
////			break;
////		case VideoFormat::NV12:
////			shader.append(NV12);
////			break;
////		case VideoFormat::NV21:
////			shader.append(NV21);
////			break;
////		case VideoFormat::YUY2:
////			shader.append(YUY2);
////			break;
////		case VideoFormat::UYVY:
////			shader.append(UYVY);
////			break;
////		default:
////			break;
////		}
////		if (m_effects & VideoRendererItem::FilterEffects)
////			shader.append(FilterMain);
////		else if (m_effects & VideoRendererItem::KernelEffects)
////			shader.append(KernelMain);
////		else
////			shader.append(SimpleMain);
////	} else {
////		shader = Rgb;
////	}
////	return shader;
//}

