#include "subtitleautoselect.hpp"

const std::array<SubtitleAutoselectInfo::Item, 4> SubtitleAutoselectInfo::info{{
    {SubtitleAutoselect::Matched, u"Matched"_q, u""_q, (int)0},
    {SubtitleAutoselect::First, u"First"_q, u""_q, (int)1},
    {SubtitleAutoselect::All, u"All"_q, u""_q, (int)2},
    {SubtitleAutoselect::EachLanguage, u"EachLanguage"_q, u""_q, (int)3}
}};
