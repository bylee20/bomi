#include "subtitleautoload.hpp"

const std::array<SubtitleAutoloadInfo::Item, 3> SubtitleAutoloadInfo::info{{
    {SubtitleAutoload::Matched, "Matched", "", (int)0},
    {SubtitleAutoload::Contain, "Contain", "", (int)1},
    {SubtitleAutoload::Folder, "Folder", "", (int)2}
}};
