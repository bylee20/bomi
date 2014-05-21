#include "videoeffect.hpp"

const std::array<VideoEffectInfo::Item, 8> VideoEffectInfo::info{{
    {VideoEffect::None, "None", "", (int)0},
    {VideoEffect::FlipV, "FlipV", "", (int)(1 << 0)},
    {VideoEffect::FlipH, "FlipH", "", (int)(1 << 1)},
    {VideoEffect::Gray, "Gray", "", (int)(1 << 2)},
    {VideoEffect::Invert, "Invert", "", (int)(1 << 3)},
    {VideoEffect::Blur, "Blur", "", (int)(1 << 4)},
    {VideoEffect::Sharpen, "Sharpen", "", (int)(1 << 5)},
    {VideoEffect::Disable, "Disable", "", (int)(1 << 8)}
}};
