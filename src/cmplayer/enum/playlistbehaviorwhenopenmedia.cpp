#include "playlistbehaviorwhenopenmedia.hpp"

const std::array<PlaylistBehaviorWhenOpenMediaInfo::Item, 3> PlaylistBehaviorWhenOpenMediaInfo::info{{
    {PlaylistBehaviorWhenOpenMedia::AppendToPlaylist, "AppendToPlaylist", "", (int)0},
    {PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist, "ClearAndAppendToPlaylist", "", (int)1},
    {PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist, "ClearAndGenerateNewPlaylist", "", (int)2}
}};
