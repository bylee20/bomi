#include "subtitledisplay.hpp"

const std::array<SubtitleDisplayInfo::Item, 2> SubtitleDisplayInfo::info{{
    {SubtitleDisplay::OnLetterbox, "OnLetterbox", "on-letterbox", (int)0},
    {SubtitleDisplay::InVideo, "InVideo", "in-video", (int)1}
}};
