#ifndef VIDEOPROCESSOR_HPP
#define VIDEOPROCESSOR_HPP

struct vf_instance;                     struct mp_image_params;
struct vf_info;                         struct mp_image;
struct MotionIntrplOption;
enum class DeintMethod;

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
    auto setMotionIntrplOption(const MotionIntrplOption &option) -> void;
signals:
    void inputInterlacedChanged();
    void outputInterlacedChanged();
    void deintMethodChanged(DeintMethod method);
    void skippingChanged(bool skipping);
    void seekRequested(int msec);
private:
    static auto open(vf_instance *vf) -> int;
    static auto queryFormat(vf_instance *vf, uint fmt) -> int;
    auto open() -> int;
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
