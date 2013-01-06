#include "videoshader.hpp"

VideoShader::VideoShader(const QGLContext *ctx)
: m_common(QGLShader::Fragment), m_filter(QGLShader::Fragment), m_kernel(QGLShader::Fragment), m_simple(QGLShader::Fragment), m_yuv(QGLShader::Fragment) {
	// **** common shader *****
	m_common.compileSourceCode(R"(
uniform float brightness, contrast;
uniform mat2 sat_hue;
uniform vec3 rgb_c;
uniform float rgb_0;
uniform float y_tan, y_b;

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
)");

	// ***** shader for no effect *****
	m_programs[0] = new Program(ctx, R"(
vec3 get_yuv(const vec2 coord);
void convert(inout vec3 yuv);

void main() {
	vec3 c = get_yuv(gl_TexCoord[0].xy);
	convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
}
	)");
// ***** shader for no effect *****
//m_programs[0] = new Program(ctx, R"(
//uniform sampler2D p1;
//vec3 get_yuv(const vec2 coord);
//void convert(inout vec3 yuv);

//void main() {
//	gl_FragColor = texture2D(p1, gl_TexCoord[0].xy);
//}
//)");

	m_rgbProgram = new Program(ctx, R"(
uniform sampler2D p1;
void main() {
	gl_FragColor = texture2D(p1, gl_TexCoord[0].xy);
}
	)");
	// ***** shader for no kernel effects *****
	m_programs[1] = new Program(ctx, R"(
vec3 get_yuv(const vec2 coord);
void apply_filter_convert(inout vec3 yuv);

void main() {
	vec3 c = get_yuv(gl_TexCoord[0].xy);
	apply_filter_convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
}
	)");

	// ***** shader for kernel effects *****
	m_programs[2] = new Program(ctx, R"(
uniform vec4 dxy;
uniform float kern_c, kern_n, kern_d;
void convert(inout vec3 yuv);
void apply_filter_convert(inout vec3 yuv);
vec3 get_yuv(const vec2 coord);
vec3 get_yuv_kernel_applied(const in vec2 coord) {
// dxy.zy   dxy.wy   dxy.xy
// dxy.zw     0      dxy.xw
//-dxy.xy  -dxy.wy  -dxy.zy
	vec3 c = get_yuv(coord)*kern_c;
	c += (get_yuv(coord + dxy.wy)+get_yuv(coord + dxy.zw)+get_yuv(coord + dxy.xw)+get_yuv(coord - dxy.wy))*kern_n;
	c += (get_yuv(coord + dxy.zy)+get_yuv(coord + dxy.xy)+get_yuv(coord - dxy.xy)+get_yuv(coord - dxy.zy))*kern_d;
	return c;
}

void main() {
	vec3 c = get_yuv_kernel_applied(gl_TexCoord[0].xy);
	apply_filter_convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
}
	)");

	for (auto &program : m_programs) {
		program->addShader(&m_common);
		program->addShader(&m_yuv);
	}

	m_codes[YV12] = R"(
uniform sampler2D p1, p2, p3;
vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(p1, coord).x;
	yuv.y = texture2D(p2, coord).x;
	yuv.z = texture2D(p3, coord).x;
	return yuv;
}
	)";
	m_codes[NV12] = R"(
uniform sampler2D p1, p2;
vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(p1, coord).x;
	yuv.yz = texture2D(p2, coord).xw;
	return yuv;
}
	)";
	m_codes[NV21] = R"(
uniform sampler2D p1, p2;
vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(p1, coord).x;
	yuv.yz = texture2D(p2, coord).wx;
	return yuv;
}
	)";
	m_codes[YUY2] = R"(
uniform sampler2D p1, p2;
vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(p1, coord).x;
	yuv.yz = texture2D(p2, coord).yw;
	return yuv;
}
	)";
	m_codes[UYVY] = R"(
uniform sampler2D p1, p2;
vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(p1, coord).a;
	yuv.yz = texture2D(p2, coord).zx;
	return yuv;
}
)";
}

VideoShader::~VideoShader() {
	delete m_programs[0];
	delete m_programs[1];
	delete m_programs[2];
	delete m_rgbProgram;
}

bool VideoShader::link(VideoFormat::Type type) {
	if (m_type == type)
		return m_type != VideoFormat::Unknown;
	if (type == VideoFormat::Unknown)
		return false;
	m_type = type;
	bool rgb = false;
	switch (type) {
	case VideoFormat::YV12:
	case VideoFormat::I420:
		m_yuv.compileSourceCode(m_codes[YV12]);
		break;
	case VideoFormat::NV12:
		m_yuv.compileSourceCode(m_codes[NV12]);
		break;
	case VideoFormat::NV21:
		m_yuv.compileSourceCode(m_codes[NV21]);
		break;
	case VideoFormat::YUY2:
		m_yuv.compileSourceCode(m_codes[YUY2]);
		break;
	case VideoFormat::UYVY:
		m_yuv.compileSourceCode(m_codes[UYVY]);
		break;
	case VideoFormat::BGRA:
	case VideoFormat::RGBA:
		rgb = true;
		break;
	default:
		return false;
	}
	if (rgb) {
		if (!m_rgbProgram->link())
			return false;
	} else {
		if (!m_yuv.isCompiled())
			return false;
		for (auto &program : m_programs)
			if (!program->link())
				return false;
	}
	qDebug() << "shader linked!";
	return true;
}
