#include "interpolator.hpp"

const std::array<InterpolatorInfo::Item, 12> InterpolatorInfo::info{{
    {Interpolator::Bilinear, u"Bilinear"_q, u"bilinear"_q, "bilinear"_b},
    {Interpolator::BicubicBS, u"BicubicBS"_q, u"b-spline"_q, "bicubic_fast"_b},
    {Interpolator::BicubicCR, u"BicubicCR"_q, u"catmull"_q, "catmull_rom"_b},
    {Interpolator::BicubicMN, u"BicubicMN"_q, u"mitchell"_q, "mitchell"_b},
    {Interpolator::Spline16, u"Spline16"_q, u"spline16"_q, "spline16"_b},
    {Interpolator::Spline36, u"Spline36"_q, u"spline36"_q, "spline36"_b},
    {Interpolator::Spline64, u"Spline64"_q, u"spline64"_q, "spline64"_b},
    {Interpolator::Lanczos2, u"Lanczos2"_q, u"lanczos2"_q, "lanczos2"_b},
    {Interpolator::Lanczos3, u"Lanczos3"_q, u"lanczos3"_q, "lanczos3"_b},
    {Interpolator::Lanczos4, u"Lanczos4"_q, u"lanczos4"_q, "lanczos4"_b},
    {Interpolator::Sharpen3, u"Sharpen3"_q, u"sharpen3"_q, "sharpen3"_b},
    {Interpolator::Sharpen5, u"Sharpen5"_q, u"sharpen5"_q, "sharpen5"_b}
}};
