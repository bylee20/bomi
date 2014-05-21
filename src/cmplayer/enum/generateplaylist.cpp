#include "generateplaylist.hpp"

const std::array<GeneratePlaylistInfo::Item, 2> GeneratePlaylistInfo::info{{
    {GeneratePlaylist::Similar, "Similar", "", (int)0},
    {GeneratePlaylist::Folder, "Folder", "", (int)1}
}};
