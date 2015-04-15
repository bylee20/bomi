#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include "audiofilter.hpp"
#include "channelmanipulation.hpp"
#include "channellayoutmap.hpp"
#include "audionormalizeroption.hpp"
#include "audioequalizer.hpp"

class AudioMixer : public AudioFilter {
public:
    AudioMixer();
    ~AudioMixer();
    auto setFormat(const AudioBufferFormat &in, const AudioBufferFormat &out) -> void;
    auto setAmplifier(float level) -> void;
    auto setEqualizer(const AudioEqualizer &eq) -> void;
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void;
    auto setSoftClip(bool soft) -> void;
    auto delay() const -> double override;
    auto run(AudioBufferPtr &in) -> AudioBufferPtr override;
    auto passthrough(const AudioBufferPtr &in) const -> bool override;
private:
    struct Data;
    Data *d;
};

#endif // AUDIOMIXER_HPP
