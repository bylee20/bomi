#include "subtitleautoload.hpp"

const std::array<SubtitleAutoloadInfo::Item, 3> SubtitleAutoloadInfo::info{{
    {SubtitleAutoload::Matched, u"Matched"_q, u""_q, (int)0},
    {SubtitleAutoload::Contain, u"Contain"_q, u""_q, (int)1},
    {SubtitleAutoload::Folder, u"Folder"_q, u""_q, (int)2}
}};
