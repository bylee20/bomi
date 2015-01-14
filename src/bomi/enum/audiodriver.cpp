#include "audiodriver.hpp"

const std::array<AudioDriverInfo::Item, 8> AudioDriverInfo::info{{
    {AudioDriver::Auto, u"Auto"_q, u""_q, (int)0},
    {AudioDriver::CoreAudio, u"CoreAudio"_q, u""_q, (int)1},
    {AudioDriver::PulseAudio, u"PulseAudio"_q, u""_q, (int)2},
    {AudioDriver::OSS, u"OSS"_q, u""_q, (int)3},
    {AudioDriver::ALSA, u"ALSA"_q, u""_q, (int)4},
    {AudioDriver::JACK, u"JACK"_q, u""_q, (int)5},
    {AudioDriver::PortAudio, u"PortAudio"_q, u""_q, (int)6},
    {AudioDriver::OpenAL, u"OpenAL"_q, u""_q, (int)7}
}};
