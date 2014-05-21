#include "adjustcolor.hpp"

const std::array<AdjustColorInfo::Item, 9> AdjustColorInfo::info{{
    {AdjustColor::Reset, "Reset", "reset", {0, 0, 0, 0}},
    {AdjustColor::BrightnessInc, "BrightnessInc", "brightness+", {1, 0, 0, 0}},
    {AdjustColor::BrightnessDec, "BrightnessDec", "brightness-", {-1, 0, 0, 0}},
    {AdjustColor::ContrastInc, "ContrastInc", "contrast+", {0, 1, 0, 0}},
    {AdjustColor::ContrastDec, "ContrastDec", "contrast-", {0, -1, 0, 0}},
    {AdjustColor::SaturationInc, "SaturationInc", "saturation+", {0, 0, 1, 0}},
    {AdjustColor::SaturationDec, "SaturationDec", "saturation-", {0, 0, -1, 0}},
    {AdjustColor::HueInc, "HueInc", "hue+", {0, 0, 0, 1}},
    {AdjustColor::HueDec, "HueDec", "hue-", {0, 0, 0, -1}}
}};
