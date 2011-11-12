!!ARBfp1.0
ATTRIB coord = fragment.texcoord[0];
OUTPUT output = result.color;
PARAM param0 = program.local[0];
PARAM coef_r = {1.16438356, 0.0, 1.59602679, 0.0625};
PARAM coef_g = {1.16438356, -0.391762290, -0.812967647, 0.5};
PARAM coef_b = {1.16438356, 2.01723214, 0.0, 0.5};
TEMP yuv;
TEX yuv.x, coord, texture[1], 2D;
SUB yuv.z, yuv.x, coef_g.a;
TEX yuv.x, coord, texture[2], 2D;
SUB yuv.w, yuv.x, coef_b.a;
MUL yuv.x, yuv.z, param0.z;
MUL yuv.y, yuv.w, param0.w;
ADD yuv.y, yuv.x, yuv.y;
MUL yuv.x, yuv.z, param0.w;
MUL yuv.w, yuv.w, param0.z;
SUB yuv.z, yuv.w, yuv.x;
TEX yuv.x, coord, texture[0], 2D;
SUB yuv.x, yuv, coef_r.a;
MUL yuv, yuv, param0.xyyz;
ADD yuv.x, yuv, coord.z;
DP3 output.r, yuv, coef_r;
DP3 output.g, yuv, coef_g;
DP3 output.b, yuv, coef_b;
MOV output.a, 1.0;
END
