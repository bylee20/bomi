varying vec2 texCoord;
#ifdef USE_BICUBIC
varying vec2 bicubicLutCoord;
#endif

/***********************************************************************/

#ifdef FRAGMENT
uniform vec2 sc2, sc3; // stride correction for texture coordinate
#ifdef USE_RECTANGLE
uniform sampler2DRect p1, p2, p3;
vec4 texture1(const in vec2 coord) { return texture2DRect(p1, coord); }
vec4 texture2(const in vec2 coord) { return texture2DRect(p2, coord*sc2); }
vec4 texture3(const in vec2 coord) { return texture2DRect(p3, coord*sc3); }
const vec4 dxy = vec4(1.0, 1.0, -1.0, 0.0);
#else
uniform sampler2D p1, p2, p3;
vec4 texture1(const in vec2 coord) { return texture2D(p1, coord); }
vec4 texture2(const in vec2 coord) { return texture2D(p2, coord*sc2); }
vec4 texture3(const in vec2 coord) { return texture2D(p3, coord*sc3); }
const vec4 dxy = vec4(1.0/WIDTH, 1.0/HEIGHT, -1.0/WIDTH, 0.0);
#endif

vec3 texel(const in vec2 coord);
#define TEXEL texel

#ifdef USE_DEINT
uniform float top_field, deint;
vec3 deinterlaced(const in vec2 coord) {
        float offset = deint*(top_field*dxy.y + dxy.y*0.5 + mod(coord.y, 2.0*dxy.y));
#if USE_DEINT == 1 // Bob
        return TEXEL(coord - vec2(0.0, offset));
#elif USE_DEINT == 2 // LinearBob
        return mix(TEXEL(coord + vec2(0.0, -offset)), TEXEL(coord + vec2(0.0, 2.0*dxy.y-offset)), offset/(2.0*dxy.y));
#endif
}
#undef TEXEL
#define TEXEL deinterlaced
#endif

#ifdef USE_BICUBIC
uniform sampler1D cubic_lut;
vec3 bicubic(const in vec2 coord) {
    // b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
    vec4 hg_x = texture1D(cubic_lut, bicubicLutCoord.x);
    vec4 hg_y = texture1D(cubic_lut, bicubicLutCoord.y);

    vec3 tex00 = TEXEL(coord + vec2(-hg_x.b, -hg_y.b)*dxy.xy);
    vec3 tex01 = TEXEL(coord + vec2(-hg_x.b,  hg_y.g)*dxy.xy);
    vec3 tex10 = TEXEL(coord + vec2( hg_x.g, -hg_y.b)*dxy.xy);
    vec3 tex11 = TEXEL(coord + vec2( hg_x.g,  hg_y.g)*dxy.xy);

    tex00 = hg_y.r*mix(tex00, tex01, hg_y.a);
    tex10 = hg_y.r*mix(tex10, tex11, hg_y.a);
    return  hg_x.r*mix(tex00, tex10, hg_x.a);
}
#undef TEXEL
#define TEXEL bicubic
#endif

#ifdef USE_KERNEL3x3
uniform float kern_c, kern_n, kern_d;
vec3 applied3x3(const in vec2 coord) {
        // dxy.zy   dxy.wy   dxy.xy
        // dxy.zw     0      dxy.xw
        //-dxy.xy  -dxy.wy  -dxy.zy
        vec3 c = TEXEL(coord)*kern_c;
        c += (TEXEL(coord + dxy.wy)+TEXEL(coord + dxy.zw)+TEXEL(coord + dxy.xw)+TEXEL(coord - dxy.wy))*kern_n;
        c += (TEXEL(coord + dxy.zy)+TEXEL(coord + dxy.xy)+TEXEL(coord - dxy.xy)+TEXEL(coord - dxy.zy))*kern_d;
        return c;
}
#undef TEXEL
#define TEXEL applied3x3
#endif

uniform mat3 mul_mat;
uniform vec3 sub_vec, add_vec;
void main() {
    vec3 tex = TEXEL(texCoord);
    tex -= sub_vec;
    tex *= mul_mat;
    tex += add_vec;
    const vec2 one = vec2(1.0, 0.0);
    gl_FragColor = tex.rgbr*one.xxxy + one.yyyx;
}
#endif

/***********************************************************************/

#ifdef VERTEX

uniform mat4 qt_Matrix;
attribute vec4 vertexPosition;
attribute vec2 textureCoordinate;
void main() {
#ifdef USE_BICUBIC
#ifdef USE_RECTANGLE
    const vec2 to_size = vec2(1.0, 1.0);
#else
    const vec2 to_size = vec2(WIDTH, HEIGHT);
#endif
    bicubicLutCoord = textureCoordinate*to_size - vec2(0.5f, 0.5f);
#endif
    texCoord = textureCoordinate;
    gl_Position = qt_Matrix * vertexPosition;
}
#endif
