#ifdef USE_RECTANGLE
const vec4 dxy = vec4(1.0, 1.0, -1.0, 0.0);
#else
const vec4 dxy = vec4(1.0/texWidth, 1.0/texHeight, -1.0/texWidth, 0.0);
#endif

#ifdef USE_KERNEL3x3
const int nCoord = 9;
#else
const int nCoord = 1;
#endif

#if defined(USE_DEINT) && USE_DEINT == 2
const int nPos = 2;
varying float deintMixes[nPos];
#else
const int nPos = 1;
#endif

const int iMC = 0*nPos;
#ifdef USE_KERNEL3x3
const int iMR = 1*nPos;
const int iML = 2*nPos;
const int iTC = 3*nPos;
const int iTL = 4*nPos;
const int iTR = 5*nPos;
const int iBR = 6*nPos;
const int iBL = 7*nPos;
const int iBC = 8*nPos;
#endif

varying vec2 texCoords0[nCoord*nPos];
#if TEX_COUNT > 1
varying vec2 texCoords1[nCoord*nPos];
#if TEX_COUNT > 2
varying vec2 texCoords2[nCoord*nPos];
#endif
#endif

/***********************************************************************/

#ifdef FRAGMENT
#ifdef USE_RECTANGLE
#define sampler2D sampler2DRect
#define texture2D texture2DRect
#endif

uniform sampler2D tex0, tex1, tex2;
vec4 texture0(const in int i) { return texture2D(tex0, texCoords0[i]); }
#if TEX_COUNT > 1
vec4 texture1(const in int i) { return texture2D(tex1, texCoords1[i]); }
#if TEX_COUNT > 2
vec4 texture2(const in int i) { return texture2D(tex2, texCoords2[i]); }
#endif
#endif

vec3 texel(const in int i);

#if defined(USE_DEINT) && USE_DEINT == 2 // LinearBob
vec3 deinterlaced(const in int i) {
    return mix(texel(i), texel(i+1), deintMixes[i]);
}
#else
#define deinterlaced texel
#endif

#ifdef USE_KERNEL3x3
uniform float kern_c, kern_n, kern_d;
#endif
vec3 filtered() {
#ifdef USE_KERNEL3x3
    vec3 c = deinterlaced(iMC)*kern_c;
    c += (deinterlaced(iTC)+deinterlaced(iBC)+deinterlaced(iML)+deinterlaced(iMR))*kern_n;
    c += (deinterlaced(iTL)+deinterlaced(iTR)+deinterlaced(iBL)+deinterlaced(iBR))*kern_d;
    return c;
#else
    return deinterlaced(iMC);
#endif
}

uniform mat3 mul_mat;
uniform vec3 sub_vec, add_vec;
void main() {
    vec3 tex = filtered();
    tex -= sub_vec;
    tex *= mul_mat;
    tex += add_vec;
    const vec2 one = vec2(1.0, 0.0);
    gl_FragColor = tex.rgbr*one.xxxy + one.yyyx;
}
#endif // FRAGMENT

/***********************************************************************/

#ifdef VERTEX
//uniform vec2 cc0, cc1, cc2; // stride Correction for texture Coordinate
uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;

#ifdef USE_DEINT
uniform float top_field, deint;
#endif
void fill_deintCoords(const in int i, const in vec2 coord) {
#ifdef USE_DEINT
    float offset = deint*(top_field*dxy.y + dxy.y*0.5 + mod(coord.y, 2.0*dxy.y));
    texCoords0[i] = coord + vec2(0.0, -offset);
#if USE_DEINT == 2
    texCoords0[i+1] = coord + vec2(0.0, 2.0*dxy.y-offset);
    deintMixes[i] = offset/(2.0*dxy.y);
#endif
#else
    texCoords0[i] = coord;
#endif
#if TEX_COUNT > 1
    texCoords1[i] = texCoords0[i]*cc1;
#if defined(USE_DEINT) && USE_DEINT == 2
    texCoords1[i+1] = texCoords0[i+1]*cc1;
#endif
#if TEX_COUNT > 2
    texCoords2[i] = texCoords0[i]*cc2;
#if defined(USE_DEINT) && USE_DEINT == 2
    texCoords2[i+1] = texCoords0[i+1]*cc2;
#endif
#endif
#endif
}

void fill_Coords(const in vec2 coord) {
    fill_deintCoords(iMC, coord);
#ifdef USE_KERNEL3x3
    fill_deintCoords(iTC, coord + dxy.wy); // top
    fill_deintCoords(iML, coord + dxy.zw); // left
    fill_deintCoords(iMR, coord + dxy.xw); // right
    fill_deintCoords(iBC, coord - dxy.wy); // bottom
    fill_deintCoords(iTL, coord + dxy.zy); // top-left
    fill_deintCoords(iTR, coord + dxy.xy); // top-right
    fill_deintCoords(iBL, coord - dxy.xy); // bottom-left
    fill_deintCoords(iBR, coord - dxy.zy); // bottom-right
#endif
}

void main() {
    fill_Coords(vCoord);
    gl_Position = vMatrix * vPosition;
}
#endif // VERTEX
