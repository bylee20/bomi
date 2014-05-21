#include "audiodriver.hpp"

const std::array<AudioDriverInfo::Item, 8> AudioDriverInfo::info{{
    {AudioDriver::Auto, "Auto", "", (int)0},
    {AudioDriver::CoreAudio, "CoreAudio", "", (int)1},
    {AudioDriver::PulseAudio, "PulseAudio", "", (int)2},
    {AudioDriver::OSS, "OSS", "", (int)3},
    {AudioDriver::ALSA, "ALSA", "", (int)4},
    {AudioDriver::JACK, "JACK", "", (int)5},
    {AudioDriver::PortAudio, "PortAudio", "", (int)6},
    {AudioDriver::OpenAL, "OpenAL", "", (int)7}
}};
