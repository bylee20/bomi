#include "deintdevice.hpp"

const std::array<DeintDeviceInfo::Item, 4> DeintDeviceInfo::info{{
    {DeintDevice::None, u"None"_q, u""_q, (int)0},
    {DeintDevice::CPU, u"CPU"_q, u""_q, (int)1},
    {DeintDevice::GPU, u"GPU"_q, u""_q, (int)2},
    {DeintDevice::OpenGL, u"OpenGL"_q, u""_q, (int)4}
}};
