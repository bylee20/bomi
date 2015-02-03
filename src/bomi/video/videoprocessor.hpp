#ifndef VIDEOPROCESSOR_HPP
#define VIDEOPROCESSOR_HPP

struct vf_instance;                     struct mp_image_params;
struct vf_info;                         struct mp_image;
class HwAcc;                            class OpenGLOffscreenContext;
enum class DeintMethod;

class VideoProcessor : public QObject {
    Q_OBJECT
public:
    VideoProcessor();
    VideoProcessor(const VideoProcessor &) = delete;
    VideoProcessor &operator = (const VideoProcessor &) = delete;
    ~VideoProcessor();
    auto initializeGL(OpenGLOffscreenContext *ctx) -> void;
    auto finalizeGL() -> void;
    auto isInputInterlaced() const -> bool;
    auto isOutputInterlaced() const -> bool;
    auto skipToNextBlackFrame() -> void;
    auto stopSkipping() -> void;
    auto isSkipping() const -> bool;
signals:
    void inputInterlacedChanged();
    void outputInterlacedChanged();
    void deintMethodChanged(DeintMethod method);
    void skippingChanged(bool skipping);
    void seekRequested(int msec);
private:
    static auto open(vf_instance *vf) -> int;
    static auto uninit(vf_instance *vf) -> void;
    static auto reconfig(vf_instance *vf,
                         mp_image_params *in, mp_image_params *out) -> int;
    static auto control(vf_instance *vf, int request, void *data) -> int;
    static auto queryFormat(vf_instance *vf, uint fmt) -> int;
    static auto filterIn(vf_instance *vf, mp_image *mpi)-> int;
    static auto filterOut(vf_instance *vf) -> int;
    struct Data;
    Data *d;
    friend auto create_vf_info() -> vf_info;
};

#endif // VIDEOPROCESSOR_HPP
