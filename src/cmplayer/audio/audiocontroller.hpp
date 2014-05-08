#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

#include "stdafx.hpp"

struct af_instance;                     struct mp_audio;
struct af_cfg;                          struct af_info;
struct mp_chmap;                        struct AudioNormalizerOption;
class ChannelLayoutMap;
enum class ClippingMethod;              enum class ChannelLayout;

class AudioFormat {
public:
    auto samplerate() const -> double { return m_samplerate; }
    auto bitrate() const -> double { return m_bitrate; }
    auto bits() const -> int { return m_bits; }
    auto type() const -> QString { return m_type; }
    auto channels() const -> QString { return m_channels; }
private:
    friend class AudioController;
    double m_samplerate = 0.0, m_bitrate = 0.0;
    int m_bits = 0;
    QString m_channels;
    QString m_type;
};

class AudioController : public QObject {
    Q_OBJECT
public:
    AudioController(QObject *parent = nullptr);
    ~AudioController();
    auto setNormalizerActivated(bool on) -> void;
    auto gain() const -> double;
    auto isTempoScalerActivated() const -> bool;
    auto isNormalizerActivated() const -> bool;
    auto setNormalizerOption(const AudioNormalizerOption &option) -> void;
    auto setClippingMethod(ClippingMethod method) -> void;
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void;
    auto setOutputChannelLayout(ChannelLayout layout) -> void;
    auto chmap() const -> mp_chmap*;
    auto inputFormat() const -> AudioFormat;
    auto outputFormat() const -> AudioFormat;
private:
    auto reinitialize(mp_audio *data) -> int;
    static auto open(af_instance *af) -> int;
    static auto test(int fmt_in, int fmt_out) -> bool;
    static auto filter(af_instance *af, mp_audio *data, int flags) -> int;
    static auto uninit(af_instance *af) -> void;
    static auto control(af_instance *af, int cmd, void *arg) -> int;
    struct Data;
    Data *d;
    friend auto create_info() -> af_info;
};

#endif // AUDIOCONTROLLER_HPP
