#include "generateplaylist.hpp"

const std::array<GeneratePlaylistInfo::Item, 2> GeneratePlaylistInfo::info{{
    {GeneratePlaylist::Similar, u"Similar"_q, u""_q, (int)0},
    {GeneratePlaylist::Folder, u"Folder"_q, u""_q, (int)1}
}};
