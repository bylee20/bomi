#include "subtitledisplay.hpp"

const std::array<SubtitleDisplayInfo::Item, 2> SubtitleDisplayInfo::info{{
    {SubtitleDisplay::OnLetterbox, u"OnLetterbox"_q, u"on-letterbox"_q, (int)0},
    {SubtitleDisplay::InVideo, u"InVideo"_q, u"in-video"_q, (int)1}
}};
