#include "videoratio.hpp"

const std::array<VideoRatioInfo::Item, 7> VideoRatioInfo::info{{
    {VideoRatio::Source, "Source", "source", -1.0},
    {VideoRatio::Window, "Window", "window", 0.0},
    {VideoRatio::_4__3, "_4__3", "4:3", 4.0/3.0},
    {VideoRatio::_16__10, "_16__10", "16:10", 16.0/10.0},
    {VideoRatio::_16__9, "_16__9", "16:9", 16.0/9.0},
    {VideoRatio::_1_85__1, "_1_85__1", "1.85:1", 1.85},
    {VideoRatio::_2_35__1, "_2_35__1", "2.35:1", 2.35}
}};
