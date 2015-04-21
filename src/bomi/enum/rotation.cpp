#include "rotation.hpp"

const std::array<RotationInfo::Item, 4> RotationInfo::info{{
    {Rotation::D0, u"D0"_q, u"0"_q, 0},
    {Rotation::D90, u"D90"_q, u"90"_q, 90},
    {Rotation::D180, u"D180"_q, u"180"_q, 180},
    {Rotation::D270, u"D270"_q, u"270"_q, 270}
}};
