#include "i420_to_rgb.hpp"

DEC_COMMON_VARS();
TEMP yuv;
yuv.x = TEX(coord, texture[1], 2D);
yuv.z = yuv.x - coef_g.a;
yuv.x = TEX(coord, texture[2], 2D);
yuv.w = yuv.x - coef_b.a;
yuv.x = yuv.z * param0.z;
yuv.y = yuv.w * param0.w;
yuv.y = yuv.x + yuv.y;
yuv.x = yuv.z * param0.w;
yuv.w = yuv.w * param0.z;
yuv.z = yuv.w - yuv.x;
yuv.x = TEX(coord, texture[0], 2D);
yuv.x = yuv - coef_r.a;
yuv = yuv * param0.xyyz;
yuv.x = yuv + coord.z;
output.r = DP3(yuv, coef_r);
output.g = DP3(yuv, coef_g);
output.b = DP3(yuv, coef_b);
output.a = 1.0;

