#include "decoderdevice.hpp"

const std::array<DecoderDeviceInfo::Item, 3> DecoderDeviceInfo::info{{
    {DecoderDevice::None, "None", "", (int)0},
    {DecoderDevice::CPU, "CPU", "", (int)1},
    {DecoderDevice::GPU, "GPU", "", (int)2}
}};
