#include "colorrange.hpp"

const std::array<ColorRangeInfo::Item, 5> ColorRangeInfo::info{{
    {ColorRange::Auto, u"Auto"_q, u"auto"_q, (int)0},
    {ColorRange::Limited, u"Limited"_q, u"limited"_q, (int)1},
    {ColorRange::Full, u"Full"_q, u"full"_q, (int)2},
    {ColorRange::Remap, u"Remap"_q, u"remap"_q, (int)3},
    {ColorRange::Extended, u"Extended"_q, u"luma"_q, (int)4}
}};
