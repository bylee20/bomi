#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

struct vo_driver;                       struct mp_image;
struct mp_image_params;                 struct vo;
class VideoFormat;                      class PlayEngine;
class DeintOption;                      class VideoRenderer;
class OpenGLOffscreenContext;           class OpenGLFramebufferObject;
enum class ColorRange;

class VideoOutput : public QObject {
    Q_OBJECT
public:
    VideoOutput(PlayEngine *engine);
    ~VideoOutput() override;
    auto prepare(void *avctx) -> void;
    auto format() const -> const VideoFormat&;
    auto setRenderer(VideoRenderer *renderer) -> void;
    auto reset() -> void;
    auto avgfps() const -> double;
    auto drawnFrames() const -> quint64;
    auto droppedFrames() const -> int;
signals:
    void droppedFramesChanged(int frames);
    void formatChanged(const VideoFormat &format);
    void avgfpsChanged(double avgfps);
    void initialized();
    void finalized();
private:
    static auto preinit(vo *out) -> int;
    static auto uninit(vo *out) -> void;
    static auto reconfig(vo *out, mp_image_params *p, int flags) -> int;
    static auto control(vo *out, quint32 request, void *data) -> int;
    static auto flipPage(vo *out) -> void;
    static auto drawImage(vo *out, mp_image *mpi) -> void;
    struct Data;
    Data *d;
    friend auto create_driver() -> vo_driver;
};

#endif // VIDEOOUTPUT_HPP
