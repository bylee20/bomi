#include "staysontop.hpp"

const std::array<StaysOnTopInfo::Item, 3> StaysOnTopInfo::info{{
    {StaysOnTop::None, u"None"_q, u"off"_q, (int)0},
    {StaysOnTop::Playing, u"Playing"_q, u"playing"_q, (int)1},
    {StaysOnTop::Always, u"Always"_q, u"always"_q, (int)2}
}};
