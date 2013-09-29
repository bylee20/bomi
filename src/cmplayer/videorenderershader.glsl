varying vec2 texCoord;
#if USE_INTERPOLATOR
varying vec2 lutInterpolatorCoord;
#endif

/***********************************************************************/

#ifdef FRAGMENT
uniform sampler2D tex;
#if USE_INTERPOLATOR
uniform sampler1D lut_interpolator;
const vec2 dxy = vec2(1.0/texWidth, 1.0/texHeight);
#endif
vec4 interpolated(const in vec2 coord) {
#if USE_INTERPOLATOR
    // b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
    vec4 hg_x = texture1D(lut_interpolator, lutInterpolatorCoord.x);
    vec4 hg_y = texture1D(lut_interpolator, lutInterpolatorCoord.y);

    vec4 tex00 = texture2D(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
    vec4 tex10 = texture2D(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
    vec4 tex01 = texture2D(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
    vec4 tex11 = texture2D(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

    tex00 = mix(tex00, tex10, hg_x.a);
    tex01 = mix(tex01, tex11, hg_x.a);
    return  mix(tex00, tex01, hg_y.a);
#else
    return texture2D(tex, coord);
#endif
}

void main() {
    gl_FragColor = interpolated(texCoord);
}

#endif
/***********************************************************************/

#ifdef VERTEX

uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;
void main() {
#if USE_INTERPOLATOR
    const vec2 size = vec2(texWidth, texHeight);
    lutInterpolatorCoord = vCoord*size - vec2(0.5, 0.5);
#endif
    texCoord = vCoord;
    gl_Position = vMatrix * vPosition;
}
#endif
