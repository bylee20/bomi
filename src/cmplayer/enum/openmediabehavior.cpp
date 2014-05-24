#include "openmediabehavior.hpp"

const std::array<OpenMediaBehaviorInfo::Item, 3> OpenMediaBehaviorInfo::info{{
    {OpenMediaBehavior::Append, "Append", "", (int)0},
    {OpenMediaBehavior::ClearAndAppend, "ClearAndAppend", "", (int)1},
    {OpenMediaBehavior::NewPlaylist, "NewPlaylist", "", (int)2}
}};
