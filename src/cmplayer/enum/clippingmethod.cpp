#include "clippingmethod.hpp"

const std::array<ClippingMethodInfo::Item, 3> ClippingMethodInfo::info{{
    {ClippingMethod::Auto, "Auto", "", (int)0},
    {ClippingMethod::Soft, "Soft", "", (int)1},
    {ClippingMethod::Hard, "Hard", "", (int)2}
}};
