#include "colorrange.hpp"

const std::array<ColorRangeInfo::Item, 5> ColorRangeInfo::info{{
    {ColorRange::Auto, "Auto", "auto", (int)0},
    {ColorRange::Limited, "Limited", "limited", (int)1},
    {ColorRange::Full, "Full", "full", (int)2},
    {ColorRange::Remap, "Remap", "remap", (int)3},
    {ColorRange::Extended, "Extended", "luma", (int)4}
}};
