#ifndef FFMPEGFILTERS_HPP
#define FFMPEGFILTERS_HPP

#include <QString>
#include <QSize>
extern "C" {
#include <video/mp_image_pool.h>
#include <video/img_format.h>
#include <libavfilter/avfiltergraph.h>
#include <libpostproc/postprocess.h>
}
#include "enum/deintmethod.hpp"
#include "mpimage.hpp"

#ifdef bool
#undef bool
#endif

class FFmpegFilterGraph {
public:
    ~FFmpegFilterGraph() { release(); }
    auto push(const MpImage &mpi) -> bool;
    auto pull() -> MpImage;
    auto initialize(const QString &opt, const QSize &s, mp_imgfmt fmt) -> bool;
    auto initialize(const QString &opt, const MpImage &mpi) -> bool
        { return initialize(opt, {mpi->w, mpi->h}, mpi->imgfmt); }
private:
    auto release() -> void;
    auto linkGraph(AVFilterInOut *&in, AVFilterInOut *&out) -> bool;
    QString m_option;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    QSize m_size = {0, 0};
    AVFilterGraph *m_graph = nullptr;
    AVFilterContext *m_src = nullptr, *m_sink = nullptr;
};

class BobDeinterlacer {
public:
    BobDeinterlacer() { m_pool = mp_image_pool_new(10); }
    ~BobDeinterlacer() { mp_image_pool_clear(m_pool); }
    auto field(DeintMethod method, const MpImage &src, bool top) const -> MpImage;
private:
    auto newImage(const MpImage &mpi) const -> MpImage;
    mp_image_pool *m_pool = nullptr;
};


#endif // FFMPEGFILTERS_HPP
