#include "dithering.hpp"

const std::array<DitheringInfo::Item, 3> DitheringInfo::info{{
    {Dithering::None, u"None"_q, u"off"_q, (int)0},
    {Dithering::Fruit, u"Fruit"_q, u"random"_q, (int)1},
    {Dithering::Ordered, u"Ordered"_q, u"ordered"_q, (int)2}
}};
