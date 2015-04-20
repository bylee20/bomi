#ifndef VIDEOPROCESSOR_HPP
#define VIDEOPROCESSOR_HPP

struct vf_instance;                     struct mp_image_params;
struct vf_info;                         struct mp_image;
struct MotionIntrplOption;
enum class DeintMethod;                 enum class ColorSpace;
enum class ColorRange;

class VideoProcessor : public QObject {
    Q_OBJECT
public:
    VideoProcessor();
    VideoProcessor(const VideoProcessor &) = delete;
    VideoProcessor &operator = (const VideoProcessor &) = delete;
    ~VideoProcessor();
    auto isInputInterlaced() const -> bool;
    auto isOutputInterlaced() const -> bool;
    auto skipToNextBlackFrame() -> void;
    auto stopSkipping() -> void;
    auto isSkipping() const -> bool;
    auto hwdec() const -> QString;
    auto setMotionIntrplOption(const MotionIntrplOption &option) -> void;
    auto inputColorSpace() const -> ColorSpace;
    auto inputColorRange() const -> ColorRange;
    auto outputColorSpace() const -> ColorSpace;
    auto outputColorRange() const -> ColorRange;
    auto setOutputColorSpace(ColorSpace space) -> void;
    auto setOutputColorRange(ColorRange range) -> void;
signals:
    void hwdecChanged(const QString &api);
    void inputInterlacedChanged();
    void outputInterlacedChanged();
    void deintMethodChanged(DeintMethod method);
    void skippingChanged(bool skipping);
    void seekRequested(int msec);
    void fpsManimulated(double fps);
    void inputColorSpaceChanged(ColorSpace space);
    void inputColorRangeChanged(ColorRange range);
    void outputColorSpaceChanged(ColorSpace space);
    void outputColorRangeChanged(ColorRange range);
private:
    static auto open(vf_instance *vf) -> int;
    static auto queryFormat(vf_instance *vf, uint fmt) -> int;
    auto uninit() -> void;
    auto reconfig(mp_image_params *in, mp_image_params *out) -> int;
    auto control(int request, void *data) -> int;
    auto filterIn(mp_image *mpi)-> int;
    auto filterOut() -> int;
    auto needsInput() const -> bool;
    struct Data;
    Data *d;
    friend auto create_vf_info() -> vf_info;
};

#endif // VIDEOPROCESSOR_HPP
