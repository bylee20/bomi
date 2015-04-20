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
    const auto pixfmt = imgfmt2pixfmt(m_imgfmt);
#define    args (tmp.toLatin1().constData())
    tmp.sprintf("width=%d:height=%d:pix_fmt=%d:time_base=1/%d:sar=1",
                m_size.width(), m_size.height(),
                pixfmt, AV_TIME_BASE);
    const auto avbuffer = avfilter_get_by_name("buffer");
    if (avfilter_graph_create_filter(&m_src, avbuffer, "src",
                                     args, nullptr, m_graph) < 0)
        return false;
    const auto avsink = avfilter_get_by_name("buffersink");
    if (avfilter_graph_create_filter(&m_sink, avsink, "sink",
                                     nullptr, nullptr, m_graph) < 0)
        return false;
    tmp.sprintf("pix_fmts=%d", pixfmt);
//    tmp = u"pix_fmts="_q;
//    for (int imgfmt = IMGFMT_START; imgfmt < IMGFMT_END; ++imgfmt) {
//        if (!IMGFMT_IS_HWACCEL(imgfmt)
//                && query_video_format(imgfmt)) {
//            const char *name = av_get_pix_fmt_name(imgfmt2pixfmt(imgfmt));
//            if (name)
//                tmp += _L(name) % '|'_q;
//        }
//    }
//    tmp.chop(1);
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

auto BobDeinterlacer::field(DeintMethod method, const MpImage &src, bool top) const -> MpImage
{
    if (src->num_planes < 1)
        return src;
    const int h = src->h;
    if (h < 4)
        return src;

    MpImage dst = std::move(newImage(src));
    const int stride = src->stride[0];
    auto in = src->planes[0], out = dst->planes[0];
    auto copy = [=] (int src, int dst)
        { memcpy(out + dst * stride, in + src * stride, stride); };

    switch (method) {
    case DeintMethod::Bob: {
        const int srcOffset = !top;
        const int count = src->h / 2;
        for (int i = 0; i < count ; ++i) {
            auto src = in + srcOffset * stride;
            memcpy(out, src, stride);
            out += stride;
            memcpy(out, src, stride);
            out += stride;
            in += 2 * stride;
        }
        break;
    } case DeintMethod::LinearBob: {
        const int count = h / 2 - 1;

        if (top) {
            copy(h - 2, h - 1);
            copy(h - 2, h - 2);
        } else {
            copy(1, 0);
            copy(h - 1, h - 1);
            in += stride;
            out += stride;
        }

        for (int i = 0; i < count ; ++i) {
            memcpy(out, in, stride);
            out += stride;
            auto in1 = in;
            auto in2 = in += stride * 2;
            for (int x = 0; x < stride; ++x)
                *out++ = (*in1++ + *in2++) / 2 ;
        }
        break;
    } case DeintMethod::CubicBob: {
        const int count = h / 2 - 2;

        if (top) {
            copy(0, 0);
            copy(0, 1);
            copy(h - 2, h - 1);
            copy(h - 2, h - 2);
            in += stride * 2;
            out += stride * 2;
        } else {
            copy(1, 0);
            copy(1, 1);
            copy(3, 2);
            copy(h - 1, h - 1);
            in += stride * 3;
            out += stride * 3;
        }

        for (int i = 0; i < count ; ++i) {
            memcpy(out, in, stride);
            out += stride;
            auto in0 = in - stride * 2;
            auto in1 = in;
            auto in2 = in += stride * 2;
            auto in3 = in + stride * 2;
            for (int x = 0; x < stride; ++x) {
                const int p0 = *in0++, p1 = *in1++, p2 = *in2++, p3 = *in3++;
                const auto a =  -p0 + 3*p1 - 3*p2 + p3;
                const auto b = 2*p0 - 5*p1 + 4*p2 - p3;
                const auto c =  -p0        +   p2;
                const auto d =        2*p1;
                *out++ = (uchar)qBound(0, (a + 2*b + 4*c + 8*d)/16, 255);
            }
        }
        break;
    } default:
        memcpy(dst->planes[0], src->planes[0], src->h*src->stride[0]);
        break;
    }
    for (int i = 1; i < src->num_planes; ++i)
        memcpy(dst->planes[i], src->planes[i],
               (src->h >> src->fmt.ys[i]) * src->stride[i]);
    return dst;
}

auto BobDeinterlacer::newImage(const MpImage &mpi) const -> MpImage
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
