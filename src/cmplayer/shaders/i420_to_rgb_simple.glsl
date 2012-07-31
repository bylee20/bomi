vec3 get_yuv(const vec2 coord);
void convert(inout vec3 yuv);

void main() {
	vec3 c = get_yuv(gl_TexCoord[0].xy);
	convert(c);
	gl_FragColor.xyz = c;
	gl_FragColor.w = 1.0;
}
