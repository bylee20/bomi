R"(
varying vec2 texCoord;

/***********************************************************************/

#ifdef FRAGMENT

uniform sampler2Dg tex0, tex1, tex2;

#define texture0(c) texture2Dg(tex0, c)
#define texture1(c) texture2Dg(tex1, c)
#define texture2(c) texture2Dg(tex2, c)

#define TEXTURE_0(i) texture2Dg(tex0, i)
#if TEX_COUNT > 1
#define TEXTURE_1(i) interpolated(tex1, (i+chroma_offset)*cc1)
#if TEX_COUNT > 2
#define TEXTURE_2(i) interpolated(tex2, (i+chroma_offset)*cc2)
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
#define TC(c) (c + dxdy.wy)
#define ML(c) (c + dxdy.zw)
#define MR(c) (c + dxdy.xw)
#define BC(c) (c - dxdy.wy)
#define TL(c) (c + dxdy.zy)
#define TR(c) (c + dxdy.xy)
#define BL(c) (c - dxdy.xy)
#define BR(c) (c - dxdy.zy)
#endif

#if USE_DEINT
uniform float top_field;
#endif
vec3 deint(const in vec2 coord) {
#if USE_DEINT
    float offset = (top_field+0.5)*dxdy.y + mod(coord.y, 2.0*dxdy.y);
#if USE_DEINT == 1
    return TEXEL(coord + vec2(0.0, -offset));
#elif USE_DEINT == 2
    return mix(TEXEL(coord + vec2(0.0, -offset)), TEXEL(coord + vec2(0.0, 2.0*dxdy.y-offset)), offset/(2.0*dxdy.y));
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
uniform vec3 add_vec;
void main() {
    const vec2 one = vec2(1.0, 0.0);
    vec3 tex = filtered(texCoord);
    tex = mul_mat*tex + add_vec;
    tex = clamp(tex, 0.0, 1.0);
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
    setLutIntCoord(vCoord);
    texCoord = vCoord;
    gl_Position = vMatrix*vPosition;
}
#endif
// VERTEX
)"
