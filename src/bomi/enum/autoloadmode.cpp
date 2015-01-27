#include "autoloadmode.hpp"

const std::array<AutoloadModeInfo::Item, 3> AutoloadModeInfo::info{{
    {AutoloadMode::Matched, u"Matched"_q, u""_q, (int)0},
    {AutoloadMode::Contain, u"Contain"_q, u""_q, (int)1},
    {AutoloadMode::Folder, u"Folder"_q, u""_q, (int)2}
}};
