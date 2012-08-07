uniform vec4 dxy;
uniform float kern_c, kern_n, kern_d;
void convert(inout vec3 yuv);
void apply_filter_convert(inout vec3 yuv);
vec3 get_yuv(const vec2 coord);
vec3 get_yuv_kernel_applied(const in vec2 coord) {
//	const vec2 tl = coord + dxy.zy;
//	const vec2 tc = coord + dxy.wy;
//	const vec2 tr = coord + dxy.xy;
//	const vec2 cl = coord + dxy.zw;
//	const vec2 cc = coord;
//	const vec2 cr = coord + dxy.xw;
//	const vec2 bl = coord - dxy.xy;
//	const vec2 bc = coord - dxy.wy;
//	const vec2 br = coord - dxy.zy;
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
