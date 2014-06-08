#include "videoratio.hpp"

const std::array<VideoRatioInfo::Item, 7> VideoRatioInfo::info{{
    {VideoRatio::Source, u"Source"_q, u"source"_q, -1.0},
    {VideoRatio::Window, u"Window"_q, u"window"_q, 0.0},
    {VideoRatio::_4__3, u"_4__3"_q, u"4:3"_q, 4.0/3.0},
    {VideoRatio::_16__10, u"_16__10"_q, u"16:10"_q, 16.0/10.0},
    {VideoRatio::_16__9, u"_16__9"_q, u"16:9"_q, 16.0/9.0},
    {VideoRatio::_1_85__1, u"_1_85__1"_q, u"1.85:1"_q, 1.85},
    {VideoRatio::_2_35__1, u"_2_35__1"_q, u"2.35:1"_q, 2.35}
}};
