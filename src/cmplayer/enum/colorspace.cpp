#include "colorspace.hpp"

const std::array<ColorSpaceInfo::Item, 5> ColorSpaceInfo::info{{
    {ColorSpace::Auto, u"Auto"_q, u"auto"_q, MP_CSP_AUTO},
    {ColorSpace::BT601, u"BT601"_q, u"bt.601"_q, MP_CSP_BT_601},
    {ColorSpace::BT709, u"BT709"_q, u"bt.709"_q, MP_CSP_BT_709},
    {ColorSpace::SMPTE240M, u"SMPTE240M"_q, u"smpte"_q, MP_CSP_SMPTE_240M},
    {ColorSpace::YCgCo, u"YCgCo"_q, u"ycgco"_q, MP_CSP_YCGCO}
}};
