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
    virtual ~FFmpegFilterGraph() { release(); }
    bool push(mp_image *mpi);
    mp_image *pull();
    bool initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt);
private:
    void release() { avfilter_graph_free(&m_graph); m_src = m_sink = nullptr; }
    bool linkGraph(AVFilterInOut *&in, AVFilterInOut *&out);
    QString m_option;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    QSize m_size = {0, 0};
    AVFilterGraph *m_graph = nullptr;
    AVFilterContext *m_src = nullptr, *m_sink = nullptr;
};

class FFmpegPostProc {
public:
    FFmpegPostProc() { m_pool = mp_image_pool_new(10); }
    virtual ~FFmpegPostProc() { release(); mp_image_pool_clear(m_pool); }
    bool process(mp_image *dest, const mp_image *src) const;
    bool initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt);
    mp_image *newImage(const mp_image *mpi) const;
private:
    void release() {
        if (m_context) pp_free_context(m_context);
        if (m_mode)    pp_free_mode(m_mode);
    }
    QString m_option;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    QSize m_size = {0, 0};
    pp_context *m_context = nullptr;
    pp_mode *m_mode = nullptr;
    mp_image_pool *m_pool = nullptr;
};


#endif // FFMPEGFILTERS_HPP
