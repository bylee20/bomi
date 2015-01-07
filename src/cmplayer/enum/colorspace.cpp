#include "colorspace.hpp"

const std::array<ColorSpaceInfo::Item, 9> ColorSpaceInfo::info{{
    {ColorSpace::Auto, u"Auto"_q, u"auto"_q, {"auto"_b, u"Autoselect"_q, u"--"_q}},
    {ColorSpace::SMPTE240M, u"SMPTE240M"_q, u"smpte"_q, "SMPTE-240M"_b},
    {ColorSpace::BT601, u"BT601"_q, u"bt.601"_q, {"BT.601"_b, u"BT.601 (SD)"_q}},
    {ColorSpace::BT709, u"BT709"_q, u"bt.709"_q, {"BT.709"_b, u"BT.709 (HD)"_q}},
    {ColorSpace::BT2020NCL, u"BT2020NCL"_q, u"bt.2020-ncl"_q, {"BT.2020-NCL"_b, u"BT.2020-NCL (UHD)"_q}},
    {ColorSpace::BT2020CL, u"BT2020CL"_q, u"bt.2020-cl"_q, {"BT.2020-CL"_b, u"BT.2020-CL (UHD)"_q}},
    {ColorSpace::RGB, u"RGB"_q, u"rgb"_q, "RGB"_b},
    {ColorSpace::XYZ, u"XYZ"_q, u"xyz"_q, "XYZ"_b},
    {ColorSpace::YCgCo, u"YCgCo"_q, u"ycgco"_q, "YCgCo"_b}
}};
