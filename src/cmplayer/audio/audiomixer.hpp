#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include "channelmanipulation.hpp"
#include "audionormalizeroption.hpp"
#include "enum/clippingmethod.hpp"
extern "C" {
#include <audio/audio.h>
}

struct AudioDataFormat {
    AudioDataFormat() { }
    AudioDataFormat(const mp_audio &mpv)
        : fps(mpv.rate), type(mpv.format), channels(mpv.channels) { }
    auto operator == (const AudioDataFormat &rhs) const -> bool
    {
        return type == rhs.type && mp_chmap_equals(&channels, &rhs.channels)
             && fps == rhs.fps;
    }
    auto operator != (const AudioDataFormat &rhs) const -> bool
        { return !operator == (rhs); }
    int fps = 0, type = 0;
    mp_chmap channels;
};

namespace detail {
template<int fmt> class AudioMixerPre;
template<int fmt> class AudioMixerPost;
struct LevelInfo {
    LevelInfo(int frames = 0)
        : frames(frames) { }
    int frames = 0; double level = 0.0;
    static auto average(const std::vector<LevelInfo> &history,
                        const LevelInfo &add) -> LevelInfo
    {
        LevelInfo total;
        for (const auto &one : history) {
            total.level += one.level*one.frames;
            total.frames += one.frames;
        }
        total.level += add.level*add.frames;
        total.frames += add.frames;
        total.level /= total.frames;
        return total;
    }
};
}

class AudioMixer {
public:
    static AudioMixer *create(const AudioDataFormat &in,
                              const AudioDataFormat &out);
    virtual ~AudioMixer() = default;
    auto gain() const -> float { return d.gain; }
    auto configure(const AudioDataFormat &in,
                   const AudioDataFormat &out) -> bool;
    auto multiplier() const -> double;
    auto delay() const -> double { return d.delay; }
    auto setNormalizer(bool on, const AudioNormalizerOption &option) -> void;
    auto setAmp(float level) -> void { d.amp = level; }
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void
        { d.map = map; d.ch_man = map(d.in.channels, d.out.channels); }
    auto setClippingMethod(ClippingMethod method) -> void;
    virtual void apply(const mp_audio *in) = 0;
    virtual void setScaler(bool on, double scale) = 0;
    virtual void setOutput(mp_audio *output) = 0;
protected:
    virtual void configured() = 0;
    AudioMixer(const AudioDataFormat &in, const AudioDataFormat &out)
        { d.in = in; d.out = out; }
    auto resetNormalizer() -> void
        { d.gain = 1.0; d.history.clear(); d.historyIt = d.history.end(); }
    struct Data {
        AudioDataFormat in, out;
        double delay = 0.0, scale = 1.0;
        float gain = 1.0, amp = 1.0;
        ClippingMethod clip = ClippingMethod::Auto;
        ClippingMethod realClip = ClippingMethod::Hard;
        bool scaleChanged = false, mix = true;
        bool normalizer = false, updateChmap = false, updateFormat = false;
        AudioNormalizerOption normalizerOption;
        std::array<int, MP_SPEAKER_ID_COUNT> ch_index_src, ch_index_dst;
        ChannelManipulation ch_man;
        ChannelLayoutMap map;
        mp_audio *output = nullptr;
        std::vector<detail::LevelInfo> history;
        typename std::vector<detail::LevelInfo>::iterator historyIt;
    };
    Data d;
    template<int fmt> friend class detail::AudioMixerPre;
    template<int fmt> friend class detail::AudioMixerPost;
};

inline auto AudioMixer::multiplier() const -> double
{ return (static_cast<double>(d.out.channels.num)/d.in.channels.num)/d.scale; }

#endif // AUDIOMIXER_HPP
