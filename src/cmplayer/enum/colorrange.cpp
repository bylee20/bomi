#include "colorrange.hpp"

const std::array<ColorRangeInfo::Item, 3> ColorRangeInfo::info{{
    {ColorRange::Auto, u"Auto"_q, u"auto"_q, {"auto"_b, u"Autoselect"_q, u"--"_q}},
    {ColorRange::Limited, u"Limited"_q, u"limited"_q, {"limited"_b, u"TV"_q, u"Limited"_q}},
    {ColorRange::Full, u"Full"_q, u"full"_q, {"full"_b, u"PC"_q, u"Full"_q}}
}};
