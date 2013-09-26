varying vec2 texCoord;
#ifdef USE_BICUBIC
varying vec2 lutBicubicCoord;
#endif
const vec2 dxy = vec2(1.0/texWidth, 1.0/texHeight);

/***********************************************************************/

#ifdef FRAGMENT
uniform sampler2D tex;
uniform sampler1D lut_bicubic;
vec4 interpolated(const in vec2 coord) {
#ifdef USE_BICUBIC
    // b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
    vec4 hg_x = texture1D(lut_bicubic, lutBicubicCoord.x);
    vec4 hg_y = texture1D(lut_bicubic, lutBicubicCoord.y);

    vec4 tex00 = texture2D(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
    vec4 tex01 = texture2D(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
    vec4 tex10 = texture2D(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
    vec4 tex11 = texture2D(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

    tex00 = hg_y.r*mix(tex00, tex01, hg_y.a);
    tex10 = hg_y.r*mix(tex10, tex11, hg_y.a);
    return  hg_x.r*mix(tex00, tex10, hg_x.a);
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
#ifdef USE_BICUBIC
    lutBicubicCoord = vCoord*vec2(texWidth, texHeight) - vec2(0.5f, 0.5f);
#endif
    texCoord = vCoord;
    gl_Position = vMatrix * vPosition;
}
#endif
