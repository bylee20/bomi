#include "colorspace.hpp"

const std::array<ColorSpaceInfo::Item, 5> ColorSpaceInfo::info{{
    {ColorSpace::Auto, "Auto", "auto", MP_CSP_AUTO},
    {ColorSpace::BT601, "BT601", "bt.601", MP_CSP_BT_601},
    {ColorSpace::BT709, "BT709", "bt.709", MP_CSP_BT_709},
    {ColorSpace::SMPTE240M, "SMPTE240M", "smpte", MP_CSP_SMPTE_240M},
    {ColorSpace::YCgCo, "YCgCo", "ycgco", MP_CSP_YCGCO}
}};
