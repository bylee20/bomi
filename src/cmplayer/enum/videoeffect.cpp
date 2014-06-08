#include "videoeffect.hpp"

const std::array<VideoEffectInfo::Item, 8> VideoEffectInfo::info{{
    {VideoEffect::None, u"None"_q, u""_q, (int)0},
    {VideoEffect::FlipV, u"FlipV"_q, u""_q, (int)(1 << 0)},
    {VideoEffect::FlipH, u"FlipH"_q, u""_q, (int)(1 << 1)},
    {VideoEffect::Gray, u"Gray"_q, u""_q, (int)(1 << 2)},
    {VideoEffect::Invert, u"Invert"_q, u""_q, (int)(1 << 3)},
    {VideoEffect::Blur, u"Blur"_q, u""_q, (int)(1 << 4)},
    {VideoEffect::Sharpen, u"Sharpen"_q, u""_q, (int)(1 << 5)},
    {VideoEffect::Disable, u"Disable"_q, u""_q, (int)(1 << 8)}
}};
