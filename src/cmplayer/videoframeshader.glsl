#ifdef USE_RECTANGLE
const vec4 dxy = vec4(1.0, 1.0, -1.0, 0.0);
#else
const vec4 dxy = vec4(1.0/texWidth, 1.0/texHeight, -1.0/texWidth, 0.0);
#endif

varying vec2 texCoord;

/***********************************************************************/

#ifdef FRAGMENT

#ifdef USE_RECTANGLE
#define sampler2D sampler2DRect
#define texture2D texture2DRect
#endif

uniform sampler2D tex0, tex1, tex2;

#define TEXTURE_0(i) texture2D(tex0, i)
#if TEX_COUNT > 1
#define TEXTURE_1(i) texture2D(tex1, i*cc1)
#if TEX_COUNT > 2
#define TEXTURE_2(i) texture2D(tex2, i*cc2)
#endif
#endif

#if (TEX_COUNT == 1)
vec3 texel(const in vec4 tex0);
#define TEXEL(i) texel(TEXTURE_0(i))
#elif (TEX_COUNT == 2)
vec3 texel(const in vec4 tex0, const in vec4 tex1);
#define TEXEL(i) texel(TEXTURE_0(i), TEXTURE_1(i))
#elif (TEX_COUNT == 3)
vec3 texel(const in vec4 tex0, const in vec4 tex1, const in vec4 tex2);
#define TEXEL(i) texel(TEXTURE_0(i), TEXTURE_1(i), TEXTURE_2(i))
#endif

#define MC(c) c
#ifdef USE_KERNEL3x3
#define TC(c) (c + dxy.wy)
#define ML(c) (c + dxy.zw)
#define MR(c) (c + dxy.xw)
#define BC(c) (c - dxy.wy)
#define TL(c) (c + dxy.zy)
#define TR(c) (c + dxy.xy)
#define BL(c) (c - dxy.xy)
#define BR(c) (c - dxy.zy)
#endif

#if USE_DEINT
uniform float top_field, deint;
#endif
vec3 deint(const in vec2 coord) {
#if USE_DEINT
    float offset = deint*((top_field+0.5)*dxy.y + mod(coord.y, 2.0*dxy.y));
#if USE_DEINT == 1
    return TEXEL(coord + vec2(0.0, -offset));
#elif USE_DEINT == 2
    return mix(TEXEL(coord + vec2(0.0, -offset)), TEXEL(coord + vec2(0.0, 2.0*dxy.y-offset)), offset/(2.0*dxy.y));
#endif
#else
    return TEXEL(coord);
#endif
}

#ifdef USE_KERNEL3x3
uniform float kern_c, kern_n, kern_d;
#endif
vec3 filtered(const in vec2 coord) {
#ifdef USE_KERNEL3x3
    vec3 c = deint(MC(coord))*kern_c;
    c += (deint(TC(coord))+deint(BC(coord))+deint(ML(coord))+deint(MR(coord)))*kern_n;
    c += (deint(TL(coord))+deint(TR(coord))+deint(BL(coord))+deint(BR(coord)))*kern_d;
    return c;
#else
    return deint(MC(coord));
#endif
}

uniform mat3 mul_mat;
uniform vec3 sub_vec, add_vec;
void main() {
    const vec2 one = vec2(1.0, 0.0);
    vec3 tex = filtered(texCoord);
    tex -= sub_vec;
    tex *= mul_mat;
    tex += add_vec;
    gl_FragColor = tex.rgbr*one.xxxy + one.yyyx;
}
#endif
// FRAGMENT

/***********************************************************************/

#ifdef VERTEX
uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;

void main() {
    texCoord = vCoord;
    gl_Position = vMatrix*vPosition;
}
#endif
// VERTEX
