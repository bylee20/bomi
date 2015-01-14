#include "movetoward.hpp"

const std::array<MoveTowardInfo::Item, 5> MoveTowardInfo::info{{
    {MoveToward::Reset, u"Reset"_q, u"reset"_q, {0, 0}},
    {MoveToward::Upward, u"Upward"_q, u"up"_q, {0, -1}},
    {MoveToward::Downward, u"Downward"_q, u"down"_q, {0, 1}},
    {MoveToward::Leftward, u"Leftward"_q, u"left"_q, {-1, 0}},
    {MoveToward::Rightward, u"Rightward"_q, u"right"_q, {1, 0}}
}};
