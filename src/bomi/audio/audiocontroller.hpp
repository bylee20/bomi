#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

struct af_instance;                     struct mp_audio;
struct af_cfg;                          struct af_info;
struct mp_chmap;                        struct AudioNormalizerOption;
class ChannelLayoutMap;                 class AudioFormat;
class AudioEqualizer;                   class AudioVisualizer;
enum class ChannelLayout;

class AudioController : public QObject {
    Q_OBJECT
public:
    AudioController(QObject *parent = nullptr);
    ~AudioController();
    auto gain() const -> double;
    auto isTempoScalerActivated() const -> bool;
    auto isNormalizerActivated() const -> bool;
    auto setNormalizerOption(const AudioNormalizerOption &option) -> void;
    auto setSoftClip(bool soft) -> void;
    auto setChannelLayoutMap(const ChannelLayoutMap &map) -> void;
    auto setOutputChannelLayout(ChannelLayout layout) -> void;
    auto setEqualizer(const AudioEqualizer &eq) -> void;
    auto chmap() const -> mp_chmap*;
    auto inputFormat() const -> AudioFormat;
    auto outputFormat() const -> AudioFormat;
    auto samplerate() const -> int;
    auto setAnalyzeSpectrum(bool on) -> void;
    auto visualizer() const -> AudioVisualizer*;
signals:
    void inputFormatChanged();
    void outputFormatChanged();
    void samplerateChanged(int sr);
    void gainChanged(double gain);
    void spectrumObtained(const QList<qreal> &data);
private:
    static auto open(af_instance *af) -> int;
    static auto test(int fmt_in, int fmt_out) -> bool;
    auto reinitialize(mp_audio *data) -> int;
    auto filter(mp_audio *data) -> int;
    auto output() -> int;
    auto uninit() -> void;
    auto control(int cmd, void *arg) -> int;
    struct Data;
    Data *d;
    friend auto create_info() -> af_info;
};

#endif // AUDIOCONTROLLER_HPP
