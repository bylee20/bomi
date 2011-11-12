#include "i420_to_rgb.hpp"

DEC_COMMON_VARS();
PARAM param1 = program.local[1];
PARAM param2 = program.local[2];
TEMP ycc, tmp;
GET_YCbCr(ycc, coord);
APPLY_COLOR_FILTER_AND_OUTPUT(ycc, tmp);

