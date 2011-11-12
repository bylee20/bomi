#include "i420_to_rgb.hpp"

#define GET_YCbCr_AND_ADD_TO(ycc, temp, center, dxdy)\
ycc.w = ycc.x;\
temp.zw = ycc.xxyz;\
temp.xy = center + dxdy;\
GET_YCbCr(ycc, temp);\
ycc.w = ycc.w + ycc.x;\
temp.zw = temp + ycc.wxyz;\
temp.xy = center - dxdy;\
GET_YCbCr(ycc, temp);\
ycc.x = ycc.w + ycc;\
ycc.yz = temp.xzwy + ycc;

#define GET_YCbCr_KERNEL_APPLIED(ycc, tmp1, tmp2)\
tmp1 = 0.0;\
GET_YCbCr_AND_ADD_TO(tmp1, ycc, coord, param3.wyxx);\
GET_YCbCr_AND_ADD_TO(tmp1, ycc, coord, param3.xwxx);\
tmp1 = tmp1 * param4.y;\
tmp2 = 0.0;\
GET_YCbCr_AND_ADD_TO(tmp2, ycc, coord, param3.zyxx);\
GET_YCbCr_AND_ADD_TO(tmp2, ycc, coord, param3.xyxx);\
tmp2 = tmp2 * param4.z;\
GET_YCbCr(ycc, coord);\
ycc = ycc * param4.x;\
ycc = ycc + tmp1;\
ycc = ycc + tmp2;

DEC_COMMON_VARS();
PARAM param1 = program.local[1];
PARAM param2 = program.local[2];
PARAM param3 = program.local[3];
PARAM param4 = program.local[4];
TEMP t1, t2, t3;
ALIAS ycc = t3;
ALIAS yuv = t1;
ALIAS rgb = t2;
GET_YCbCr_KERNEL_APPLIED(ycc, t1, t2);
APPLY_COLOR_FILTER_AND_OUTPUT(ycc, t1);

