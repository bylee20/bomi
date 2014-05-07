#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

// CAUTION: NEVER CALL THIS CLASS FROM Qt's GUI

struct vo_driver;
class VideoFormat;            class PlayEngine;
struct mp_image;            class DeintOption;
class VideoRendererItem;    class HwAcc;
enum class DeintMethod;

class VideoOutput : public QObject {
    Q_OBJECT
public:
    VideoOutput(PlayEngine *engine);
    ~VideoOutput() override;
    auto prepare(void *avctx) -> void;
    auto release() -> void;
    auto format() const -> const VideoFormat&;
    auto setRenderer(VideoRendererItem *renderer) -> void;
    auto output(const QImage &image) -> void;
    auto setHwAcc(HwAcc *acc) -> void;
    static auto queryFormat(struct vo *vo, quint32 format) -> int;
signals:
    void formatChanged(VideoFormat format);
private:
    static auto preinit(struct vo *vo) -> int;
    static auto uninit(struct vo */*vo*/) -> void {}
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
