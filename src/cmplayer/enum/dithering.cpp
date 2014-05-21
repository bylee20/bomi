#include "dithering.hpp"

const std::array<DitheringInfo::Item, 3> DitheringInfo::info{{
    {Dithering::None, "None", "off", (int)0},
    {Dithering::Fruit, "Fruit", "random", (int)1},
    {Dithering::Ordered, "Ordered", "ordered", (int)2}
}};
