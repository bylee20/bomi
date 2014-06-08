#include "interpolatortype.hpp"

const std::array<InterpolatorTypeInfo::Item, 11> InterpolatorTypeInfo::info{{
    {InterpolatorType::Bilinear, u"Bilinear"_q, u"bilinear"_q, (int)0},
    {InterpolatorType::BicubicBS, u"BicubicBS"_q, u"b-spline"_q, (int)1},
    {InterpolatorType::BicubicCR, u"BicubicCR"_q, u"catmull"_q, (int)2},
    {InterpolatorType::BicubicMN, u"BicubicMN"_q, u"mitchell"_q, (int)3},
    {InterpolatorType::Spline16, u"Spline16"_q, u"spline16"_q, (int)4},
    {InterpolatorType::Spline36, u"Spline36"_q, u"spline36"_q, (int)5},
    {InterpolatorType::Spline64, u"Spline64"_q, u"spline64"_q, (int)6},
    {InterpolatorType::LanczosFast, u"LanczosFast"_q, u"lanczos-fast"_q, (int)7},
    {InterpolatorType::Lanczos2, u"Lanczos2"_q, u"lancoz2"_q, (int)8},
    {InterpolatorType::Lanczos3, u"Lanczos3"_q, u"lanczos3"_q, (int)9},
    {InterpolatorType::Lanczos4, u"Lanczos4"_q, u"lanczos4"_q, (int)10}
}};
