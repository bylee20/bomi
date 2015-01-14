#include "dithering.hpp"

const std::array<DitheringInfo::Item, 3> DitheringInfo::info{{
    {Dithering::None, u"None"_q, u"off"_q, "no"},
    {Dithering::Fruit, u"Fruit"_q, u"random"_q, "fruit"},
    {Dithering::Ordered, u"Ordered"_q, u"ordered"_q, "ordered"}
}};
