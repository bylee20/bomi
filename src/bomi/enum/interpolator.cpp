#include "interpolator.hpp"

const std::array<InterpolatorInfo::Item, 6> InterpolatorInfo::info{{
    {Interpolator::Bilinear, u"Bilinear"_q, u"bilinear"_q, "bilinear"_b},
    {Interpolator::Bicubic, u"Bicubic"_q, u"bicubic"_q, "mitchell"_b},
    {Interpolator::Spline, u"Spline"_q, u"spline"_q, "spline"_b},
    {Interpolator::Lanczos, u"Lanczos"_q, u"lanczos"_q, "lanczos"_b},
    {Interpolator::EwaLanczos, u"EwaLanczos"_q, u"ewa-lanczos"_q, "ewa_lanczos"_b},
    {Interpolator::Sharpen, u"Sharpen"_q, u"sharpen"_q, "sharpen"_b}
}};
