#include "deintmethod.hpp"

const std::array<DeintMethodInfo::Item, 5> DeintMethodInfo::info{{
    {DeintMethod::None, u"None"_q, u""_q, (int)0},
    {DeintMethod::Bob, u"Bob"_q, u""_q, (int)1},
    {DeintMethod::LinearBob, u"LinearBob"_q, u""_q, (int)2},
    {DeintMethod::CubicBob, u"CubicBob"_q, u""_q, (int)3},
    {DeintMethod::Yadif, u"Yadif"_q, u""_q, (int)4}
}};
