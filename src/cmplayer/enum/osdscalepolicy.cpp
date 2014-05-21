#include "osdscalepolicy.hpp"

const std::array<OsdScalePolicyInfo::Item, 3> OsdScalePolicyInfo::info{{
    {OsdScalePolicy::Width, "Width", "", (int)0},
    {OsdScalePolicy::Height, "Height", "", (int)1},
    {OsdScalePolicy::Diagonal, "Diagonal", "", (int)2}
}};
