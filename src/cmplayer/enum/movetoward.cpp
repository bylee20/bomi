#include "movetoward.hpp"

const std::array<MoveTowardInfo::Item, 5> MoveTowardInfo::info{{
    {MoveToward::Reset, "Reset", "reset", {0, 0}},
    {MoveToward::Upward, "Upward", "up", {0, -1}},
    {MoveToward::Downward, "Downward", "down", {0, 1}},
    {MoveToward::Leftward, "Leftward", "left", {-1, 0}},
    {MoveToward::Rightward, "Rightward", "right", {1, 0}}
}};
