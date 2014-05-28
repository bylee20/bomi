#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H

struct vf_instance;                     struct mp_image_params;
struct vf_info;                         struct mp_image;
class HwAcc;                            class OpenGLOffscreenContext;
enum class DeintMethod;

class VideoFilter : public QObject {
    Q_OBJECT
public:
    VideoFilter();
    VideoFilter(const VideoFilter &) = delete;
    VideoFilter &operator = (const VideoFilter &) = delete;
    ~VideoFilter();
    auto setHwAcc(HwAcc *acc) -> void;
    auto initializeGL(OpenGLOffscreenContext *ctx) -> void;
    auto finalizeGL() -> void;
signals:
    void deintMethodChanged(DeintMethod method);
private:
    static auto open(vf_instance *vf) -> int;
    static auto uninit(vf_instance *vf) -> void;
    static auto reconfig(vf_instance *vf,
                         mp_image_params *in, mp_image_params *out) -> int;
    static auto control(vf_instance *vf, int request, void *data) -> int;
    static auto queryFormat(vf_instance *vf, uint fmt) -> int;
    static auto filter(vf_instance *vf, mp_image *mpi)-> int;
    struct Data;
    Data *d;
    friend auto create_vf_info() -> vf_info;
};

#endif // VIDEOFILTER_H
