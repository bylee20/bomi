uniform sampler2D y, u, v;
uniform float brightness, contrast;
uniform mat2 sat_hue;
uniform vec3 rgb_c;
uniform float rgb_0;
uniform float y_tan, y_b;

vec3 get_yuv(const vec2 coord) {
	vec3 yuv;
	yuv.x = texture2D(y, coord).x;
	yuv.y = texture2D(u, coord).x;
	yuv.z = texture2D(v, coord).x;
	return yuv;
}

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

