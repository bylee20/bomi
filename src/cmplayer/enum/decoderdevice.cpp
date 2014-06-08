#include "decoderdevice.hpp"

const std::array<DecoderDeviceInfo::Item, 3> DecoderDeviceInfo::info{{
    {DecoderDevice::None, u"None"_q, u""_q, (int)0},
    {DecoderDevice::CPU, u"CPU"_q, u""_q, (int)1},
    {DecoderDevice::GPU, u"GPU"_q, u""_q, (int)2}
}};
