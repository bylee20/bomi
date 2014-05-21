#include "subtitleautoselect.hpp"

const std::array<SubtitleAutoselectInfo::Item, 4> SubtitleAutoselectInfo::info{{
    {SubtitleAutoselect::Matched, "Matched", "", (int)0},
    {SubtitleAutoselect::First, "First", "", (int)1},
    {SubtitleAutoselect::All, "All", "", (int)2},
    {SubtitleAutoselect::EachLanguage, "EachLanguage", "", (int)3}
}};
