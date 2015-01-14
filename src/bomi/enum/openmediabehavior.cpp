#include "openmediabehavior.hpp"

const std::array<OpenMediaBehaviorInfo::Item, 3> OpenMediaBehaviorInfo::info{{
    {OpenMediaBehavior::Append, u"Append"_q, u""_q, (int)0},
    {OpenMediaBehavior::ClearAndAppend, u"ClearAndAppend"_q, u""_q, (int)1},
    {OpenMediaBehavior::NewPlaylist, u"NewPlaylist"_q, u""_q, (int)2}
}};
