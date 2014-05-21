#include "interpolatortype.hpp"

const std::array<InterpolatorTypeInfo::Item, 11> InterpolatorTypeInfo::info{{
    {InterpolatorType::Bilinear, "Bilinear", "bilinear", (int)0},
    {InterpolatorType::BicubicBS, "BicubicBS", "b-spline", (int)1},
    {InterpolatorType::BicubicCR, "BicubicCR", "catmull", (int)2},
    {InterpolatorType::BicubicMN, "BicubicMN", "mitchell", (int)3},
    {InterpolatorType::Spline16, "Spline16", "spline16", (int)4},
    {InterpolatorType::Spline36, "Spline36", "spline36", (int)5},
    {InterpolatorType::Spline64, "Spline64", "spline64", (int)6},
    {InterpolatorType::LanczosFast, "LanczosFast", "lanczos-fast", (int)7},
    {InterpolatorType::Lanczos2, "Lanczos2", "lancoz2", (int)8},
    {InterpolatorType::Lanczos3, "Lanczos3", "lanczos3", (int)9},
    {InterpolatorType::Lanczos4, "Lanczos4", "lanczos4", (int)10}
}};
