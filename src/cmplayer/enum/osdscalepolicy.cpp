#include "osdscalepolicy.hpp"

const std::array<OsdScalePolicyInfo::Item, 3> OsdScalePolicyInfo::info{{
    {OsdScalePolicy::Width, u"Width"_q, u""_q, (int)0},
    {OsdScalePolicy::Height, u"Height"_q, u""_q, (int)1},
    {OsdScalePolicy::Diagonal, u"Diagonal"_q, u""_q, (int)2}
}};
