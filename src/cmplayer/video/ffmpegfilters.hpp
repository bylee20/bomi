#ifndef FFMPEGFILTERS_HPP
#define FFMPEGFILTERS_HPP

#include "stdafx.hpp"

extern "C" {
#include <video/mp_image_pool.h>
#include <video/img_format.h>
#include <libavfilter/avfiltergraph.h>
#include <libpostproc/postprocess.h>
}

class FFmpegFilterGraph {
public:
    ~FFmpegFilterGraph() { release(); }
    auto push(mp_image *mpi) -> bool;
    auto pull() -> mp_image*;
    auto initialize(const QString &option, const QSize &size,
                    mp_imgfmt imgfmt) -> bool;
private:
    auto release() -> void;
    auto linkGraph(AVFilterInOut *&in, AVFilterInOut *&out) -> bool;
    QString m_option;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    QSize m_size = {0, 0};
    AVFilterGraph *m_graph = nullptr;
    AVFilterContext *m_src = nullptr, *m_sink = nullptr;
};

class FFmpegPostProc {
public:
    FFmpegPostProc() { m_pool = mp_image_pool_new(10); }
    ~FFmpegPostProc() { release(); mp_image_pool_clear(m_pool); }
    auto process(mp_image *dest, const mp_image *src) const -> bool;
    auto initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt) -> bool;
    auto newImage(const mp_image *mpi) const -> mp_image*;
private:
    auto release() -> void;
    QString m_option;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    QSize m_size = {0, 0};
    pp_context *m_context = nullptr;
    pp_mode *m_mode = nullptr;
    mp_image_pool *m_pool = nullptr;
};


#endif // FFMPEGFILTERS_HPP
