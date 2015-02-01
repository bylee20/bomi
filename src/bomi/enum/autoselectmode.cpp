#include "autoselectmode.hpp"

const std::array<AutoselectModeInfo::Item, 4> AutoselectModeInfo::info{{
    {AutoselectMode::Matched, u"Matched"_q, u""_q, (int)0},
    {AutoselectMode::First, u"First"_q, u""_q, (int)1},
    {AutoselectMode::All, u"All"_q, u""_q, (int)2},
    {AutoselectMode::EachLanguage, u"EachLanguage"_q, u""_q, (int)3}
}};
