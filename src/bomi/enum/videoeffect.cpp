#include "videoeffect.hpp"

const std::array<VideoEffectInfo::Item, 7> VideoEffectInfo::info{{
    {VideoEffect::None, u"None"_q, u""_q, (int)0},
    {VideoEffect::FlipV, u"FlipV"_q, u""_q, (int)(1 << 0)},
    {VideoEffect::FlipH, u"FlipH"_q, u""_q, (int)(1 << 1)},
    {VideoEffect::Remap, u"Remap"_q, u""_q, (int)(1 << 4)},
    {VideoEffect::Gray, u"Gray"_q, u""_q, (int)(1 << 2)},
    {VideoEffect::Invert, u"Invert"_q, u""_q, (int)(1 << 3)},
    {VideoEffect::Disable, u"Disable"_q, u""_q, (int)(1 << 8)}
}};
