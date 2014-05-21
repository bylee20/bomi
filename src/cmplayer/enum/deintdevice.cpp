#include "deintdevice.hpp"

const std::array<DeintDeviceInfo::Item, 4> DeintDeviceInfo::info{{
    {DeintDevice::None, "None", "", (int)0},
    {DeintDevice::CPU, "CPU", "", (int)1},
    {DeintDevice::GPU, "GPU", "", (int)2},
    {DeintDevice::OpenGL, "OpenGL", "", (int)4}
}};
