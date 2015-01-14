#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include "audiofilter.hpp"
#include "channelmanipulation.hpp"
#include "channellayoutmap.hpp"
#include "audionormalizeroption.hpp"
#include "enum/clippingmethod.hpp"

class AudioMixer : public AudioFilter {
public:
    AudioMixer();
    auto setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void;
    auto setAmplifier(float level) -> void { m_amp = level; }
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void
        { m_map = map; m_ch_man = map(m_in.channels(), m_out.channels()); }
    auto setClippingMethod(ClippingMethod method) -> void;
    auto run(AudioBufferPtr in) -> AudioBufferPtr;
protected:
    AudioBufferFormat m_in, m_out;
    float m_amp = 1.0;
    ClippingMethod m_clip = ClippingMethod::Auto;
    ClippingMethod m_realClip = ClippingMethod::Hard;
    bool m_mix = true;
    bool m_updateChmap = false, m_updateFormat = false;
    std::array<int, MP_SPEAKER_ID_COUNT> m_ch_index_src, m_ch_index_dst;
    ChannelManipulation m_ch_man;
    ChannelLayoutMap m_map;
};

#endif // AUDIOMIXER_HPP
