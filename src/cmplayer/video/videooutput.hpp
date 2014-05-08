#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"

struct vo_driver;                       struct mp_image;
struct mp_image_params;
class VideoFormat;                      class PlayEngine;
class DeintOption;                      class VideoRendererItem;
class OpenGLOffscreenContext;           class OpenGLFramebufferObject;
enum class ColorRange;

class VideoOutput : public QObject {
    Q_OBJECT
public:
    VideoOutput(PlayEngine *engine);
    ~VideoOutput() override;
    auto prepare(void *avctx) -> void;
    auto format() const -> const VideoFormat&;
    auto setRenderer(VideoRendererItem *renderer) -> void;
    auto initializeGL(OpenGLOffscreenContext *gl) -> void;
    auto finalizeGL() -> void;
    auto setColorRange(ColorRange range) -> void;
    auto reset() -> void;
    auto avgfps() const -> double;
    auto drawnFrames() const -> quint64;
    auto droppedFrames() const -> int;
signals:
    void droppedFramesChanged(int frames);
    void formatChanged(const VideoFormat &format);
    void avgfpsChanged(double avgfps);
private:
    static auto preinit(struct vo *vo) -> int;
    static auto reconfig(struct vo *out, mp_image_params *p, int flags) -> int;
    static auto control(struct vo *vo, quint32 request, void *data) -> int;
    static auto drawOsd(struct vo *vo, struct osd_state *osd) -> void;
    static auto flipPage(struct vo *vo) -> void;
    static auto drawImage(struct vo *vo, mp_image *mpi) -> void;
    struct Data;
    Data *d;
    friend auto create_driver() -> vo_driver;
};

#endif // VIDEOOUTPUT_HPP
