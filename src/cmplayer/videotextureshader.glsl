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
#ifdef USE_LANCZOS
    return texture2D(tex, coord);
#else
    return texture2D(tex, coord);
#endif
#endif
}

void main() {
    gl_FragColor = interpolated(texCoord);
}


//const float pi = 3.1415926535897932384626433832795;
//// Lanczos lobes
//const float a = 3.0;

//float sinc(float x) {
//   float ptx = pi * x;
//   return sin(ptx) / ptx;
//}

//float lanczos(float x) {
//   if (x == 0.0)
//      return 1.0;
//   if (abs(x) < a)
//      return sinc(x) * sinc(x / a);

//   return 0.0;
//}

//void main(void) {
//   vec2 coord = texCoord * vec2(texWidth, texHeight) - vec2(0.5, 0.5);
//   ivec2 ic = ivec2(coord);
//   vec4 val = vec4(0.0);
//   float contrib = 0.0;
//   for (int y = -1; y < 3; y++) {
//      for (int x = -1; x < 3; x++) {
//	 vec2 d = vec2(ic + ivec2(x, y));
//	 vec2 e = abs(coord - d - vec2(0.5));
//	 float weight = lanczos(e.x) * lanczos(e.y);
//	 contrib += weight;

//	 val += texture2D(tex, (d + 0.5) *dxy) * weight;
//      }
//   }
//   gl_FragColor = val / contrib;
//}

#endif
/***********************************************************************/

#ifdef VERTEX

uniform mat4 vMatrix;
attribute vec4 vPosition;
attribute vec2 vCoord;
void main() {
#ifdef USE_BICUBIC
    lutBicubicCoord = vCoord*vec2(texWidth, texHeight) - vec2(0.5, 0.5);
#endif
    texCoord = vCoord;
    gl_Position = vMatrix * vPosition;
}
#endif
