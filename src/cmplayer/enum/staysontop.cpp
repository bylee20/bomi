#include "staysontop.hpp"

const std::array<StaysOnTopInfo::Item, 3> StaysOnTopInfo::info{{
    {StaysOnTop::None, "None", "off", (int)0},
    {StaysOnTop::Playing, "Playing", "playing", (int)1},
    {StaysOnTop::Always, "Always", "always", (int)2}
}};
