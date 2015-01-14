#include "clippingmethod.hpp"

const std::array<ClippingMethodInfo::Item, 3> ClippingMethodInfo::info{{
    {ClippingMethod::Auto, u"Auto"_q, u""_q, (int)0},
    {ClippingMethod::Soft, u"Soft"_q, u""_q, (int)1},
    {ClippingMethod::Hard, u"Hard"_q, u""_q, (int)2}
}};
