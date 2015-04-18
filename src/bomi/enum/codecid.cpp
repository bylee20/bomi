#include "codecid.hpp"

const std::array<CodecIdInfo::Item, 8> CodecIdInfo::info{{
    {CodecId::Invalid, u"Invalid"_q, u""_q, u""_q},
    {CodecId::Mpeg1, u"Mpeg1"_q, u""_q, u"mpeg1video"_q},
    {CodecId::Mpeg2, u"Mpeg2"_q, u""_q, u"mpeg2video"_q},
    {CodecId::Mpeg4, u"Mpeg4"_q, u""_q, u"mpeg4"_q},
    {CodecId::H264, u"H264"_q, u""_q, u"h264"_q},
    {CodecId::Vc1, u"Vc1"_q, u""_q, u"vc1"_q},
    {CodecId::Wmv3, u"Wmv3"_q, u""_q, u"wmv3"_q},
    {CodecId::Hevc, u"Hevc"_q, u""_q, u"hevc"_q}
}};
