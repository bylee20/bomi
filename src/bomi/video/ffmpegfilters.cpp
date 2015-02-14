#include "ffmpegfilters.hpp"
#include "global.hpp"
extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <video/mp_image.h>
#include <video/fmt-conversion.h>
}

static auto null_mp_image(void *arg, void(*free)(void*)) -> mp_image*
    { static mp_image null; return mp_image_new_custom_ref(&null, arg, free); }

auto query_video_format(quint32 format) -> int;

auto FFmpegFilterGraph::push(const MpImage &in) -> bool
{
    Q_ASSERT(m_imgfmt == in->imgfmt && m_size == QSize(in->w, in->h));
    if (!m_graph)
        return false;
    auto src = m_src->outputs[0];
    auto frame = av_frame_alloc();
    mp_image_copy_fields_to_av_frame(frame, const_cast<mp_image*>(in.data()));
    int flags = AV_BUFFER_FLAG_READONLY;
    auto freeMpImage = [](void *in, uint8_t*) { delete static_cast<MpImage*>(in); };
    for (int n = 0; n < in->num_planes; ++n) {
        auto plane = new MpImage(in);
        const size_t size = in->stride[n] * in->h;
        frame->buf[n] = av_buffer_create(in->planes[n], size,
                                         freeMpImage, plane, flags);
    }
    if (in->pts == MP_NOPTS_VALUE)
        frame->pts = AV_NOPTS_VALUE;
    else
        frame->pts = in->pts * av_q2d(av_inv_q(src->time_base));
    frame->sample_aspect_ratio = src->sample_aspect_ratio;
    const bool ok = (av_buffersrc_add_frame(m_src, frame) >= 0);
    av_frame_free(&frame);
    return ok;
}

auto FFmpegFilterGraph::pull() -> MpImage
{
    if (!m_graph)
        return MpImage();
    auto frame = av_frame_alloc();
    const auto err = av_buffersink_get_frame(m_sink, frame);
    if (err < 0) {
        av_frame_free(&frame);
        return MpImage();
    }
    auto freeAvFrame = [](void *frame) { av_frame_free((AVFrame**)&frame); };
    auto mpi = null_mp_image(frame, freeAvFrame);
    mp_image_copy_fields_from_av_frame(mpi, frame);
    return MpImage::wrap(mpi);
}

auto FFmpegFilterGraph::linkGraph(AVFilterInOut *&in,
                                  AVFilterInOut *&out) -> bool
{
    QString tmp;
#define    args (tmp.toLocal8Bit().constData())
    tmp.sprintf("width=%d:height=%d:pix_fmt=%d:time_base=1/%d:sar=1",
                m_size.width(), m_size.height(),
                imgfmt2pixfmt(m_imgfmt), AV_TIME_BASE);
    const auto avbuffer = avfilter_get_by_name("buffer");
    if (avfilter_graph_create_filter(&m_src, avbuffer, "src",
                                     args, nullptr, m_graph) < 0)
        return false;
    const auto avsink = avfilter_get_by_name("buffersink");
    if (avfilter_graph_create_filter(&m_sink, avsink, "sink",
                                     nullptr, nullptr, m_graph) < 0)
        return false;
    tmp = u"pix_fmts="_q;
    for (int imgfmt = IMGFMT_START; imgfmt < IMGFMT_END; ++imgfmt) {
        if (!IMGFMT_IS_HWACCEL(imgfmt)
                && query_video_format(imgfmt)) {
            const char *name = av_get_pix_fmt_name(imgfmt2pixfmt(imgfmt));
            if (name)
                tmp += _L(name) % '|'_q;
        }
    }
    tmp.chop(1);
    AVFilterContext *format = nullptr;
    const auto avformat = avfilter_get_by_name("format");
    if (avfilter_graph_create_filter(&format, avformat, "format",
                                     args, nullptr, m_graph) < 0)
        return false;
    if (avfilter_link(format, 0, m_sink, 0) < 0)
        return false;
    out->name = av_strdup("in");
    out->filter_ctx = m_src;
    in->name = av_strdup("out");
    in->filter_ctx = format;
    tmp.sprintf("flags=%d", SWS_BICUBIC);
    m_graph->scale_sws_opts = av_strdup(args);
    if (avfilter_graph_parse_ptr(m_graph, m_option.toLatin1().constData(),
                                 &in, &out, nullptr) < 0)
        return false;
    if (avfilter_graph_config(m_graph, nullptr) < 0)
        return false;
    Q_ASSERT(m_sink->nb_inputs == 1);
    Q_ASSERT(m_src->nb_outputs == 1);
#undef args
    return true;
}

auto FFmpegFilterGraph::initialize(const QString &option, const QSize &size,
                                   mp_imgfmt imgfmt) -> bool
{
    avfilter_register_all();
    if (option == m_option && m_size == size && m_imgfmt == imgfmt)
        return m_graph;
    m_option = option; m_size = size; m_imgfmt = imgfmt;
    release();
    if (option.isEmpty() || size.isEmpty()
            || imgfmt == IMGFMT_NONE || IMGFMT_IS_HWACCEL(imgfmt))
        return false;
    auto out = avfilter_inout_alloc();
    auto in = avfilter_inout_alloc();
    m_graph = avfilter_graph_alloc();
    if (!linkGraph(in, out))
        release();
    avfilter_inout_free(&out);
    avfilter_inout_free(&in);
    return m_graph;
}

auto FFmpegFilterGraph::release() -> void
{
    avfilter_graph_free(&m_graph);
    m_src = m_sink = nullptr;
}

/******************************************************************************/

auto FFmpegPostProc::process(MpImage &di, const MpImage&si) const -> bool
{
    if (!m_context)
        return false;
    const uint8_t *src[3]  = {si->planes[0], si->planes[1], si->planes[2]};
          uint8_t *dst[3]  = {di->planes[0], di->planes[1], di->planes[2]};
    const int srcStride[3] = {si->stride[0], si->stride[1], si->stride[2]};
    const int dstStride[3] = {di->stride[0], di->stride[1], di->stride[2]};
    pp_postprocess(src, srcStride, dst, dstStride, si->w, si->h,
                   nullptr, 0, m_mode, m_context, si->pict_type);
    return true;
}

auto FFmpegPostProc::initialize(const QString &option, const QSize &size,
                                mp_imgfmt imgfmt) -> bool
{
    if (m_option == option && m_size == size && m_imgfmt == imgfmt)
        return m_context;
    release();
    m_option = option;
    m_size = size;
    m_imgfmt = imgfmt;
    if (m_option.isEmpty() || m_size.isEmpty())
        return false;
    int flags = PP_CPU_CAPS_AUTO;
    switch (imgfmt) {
    case IMGFMT_420P:
        flags |= PP_FORMAT_420;
        break;
    case IMGFMT_411P:
        flags |= PP_FORMAT_411;
        break;
    case IMGFMT_422P:
        flags |= PP_FORMAT_422;
        break;
    case IMGFMT_444P:
        flags |= PP_FORMAT_444;
        break;
    default:
        return false;
    }
    m_mode = pp_get_mode_by_name_and_quality(m_option.toLatin1().constData(),
                                             PP_QUALITY_MAX);
    if (!m_mode)
        return false;
    m_context = pp_get_context(size.width(), size.height(), flags);
    return m_context;
}

auto FFmpegPostProc::newImage(const MpImage &mpi) const -> MpImage
{
    auto tmp = mp_image_pool_get(m_pool, mpi->imgfmt, mpi->stride[0], mpi->h);
    auto img = mp_image_new_ref(tmp);
    talloc_free(tmp);
    img->w = mpi->w;
    img->h = mpi->h;
    img->stride[0] = mpi->stride[0];
    img->stride[1] = mpi->stride[1];
    img->stride[2] = mpi->stride[2];
    img->stride[3] = mpi->stride[3];
    mp_image_copy_attributes(img, (mp_image*)mpi.data());
    return MpImage::wrap(img);
}

auto FFmpegPostProc::release() -> void
{
    if (m_context)
        pp_free_context(m_context);
    if (m_mode)
        pp_free_mode(m_mode);
}
