#include "deintmethod.hpp"

const std::array<DeintMethodInfo::Item, 8> DeintMethodInfo::info{{
    {DeintMethod::None, "None", "", (int)0},
    {DeintMethod::Bob, "Bob", "", (int)1},
    {DeintMethod::LinearBob, "LinearBob", "", (int)2},
    {DeintMethod::CubicBob, "CubicBob", "", (int)3},
    {DeintMethod::Median, "Median", "", (int)4},
    {DeintMethod::LinearBlend, "LinearBlend", "", (int)5},
    {DeintMethod::Yadif, "Yadif", "", (int)6},
    {DeintMethod::MotionAdaptive, "MotionAdaptive", "", (int)7}
}};
