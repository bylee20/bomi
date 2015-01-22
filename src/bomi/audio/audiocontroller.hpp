#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

struct af_instance;                     struct mp_audio;
struct af_cfg;                          struct af_info;
struct mp_chmap;                        struct AudioNormalizerOption;
class ChannelLayoutMap;                 class AudioFormat;
class AudioEqualizer;
enum class ClippingMethod;              enum class ChannelLayout;

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
    auto setEqualizer(const AudioEqualizer &eq) -> void;
    auto chmap() const -> mp_chmap*;
    auto inputFormat() const -> AudioFormat;
    auto outputFormat() const -> AudioFormat;
    auto samplerate() const -> int;
signals:
    void inputFormatChanged();
    void outputFormatChanged();
    void samplerateChanged(int sr);
    void gainChanged(double gain);
private:
    auto reinitialize(mp_audio *data) -> int;
    static auto open(af_instance *af) -> int;
    static auto test(int fmt_in, int fmt_out) -> bool;
    static auto filter(af_instance *af, mp_audio *data) -> int;
    static auto uninit(af_instance *af) -> void;
    static auto control(af_instance *af, int cmd, void *arg) -> int;
    struct Data;
    Data *d;
    friend auto create_info() -> af_info;
};

#endif // AUDIOCONTROLLER_HPP
