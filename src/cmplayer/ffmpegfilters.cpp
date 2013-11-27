#include "ffmpegfilters.hpp"
#include "videooutput.hpp"
extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <video/mp_image.h>
#include <video/fmt-conversion.h>
#include <mpvcore/cpudetect.h>
}

mp_image *nullMpImage(void *arg = nullptr, void(*free)(void*) = nullptr);
mp_image *nullMpImage(uint imgfmt, int width, int height, void *arg = nullptr, void(*free)(void*) = nullptr);

bool FFmpegFilterGraph::push(mp_image *in) {
	Q_ASSERT(m_imgfmt == in->imgfmt && m_size == QSize(in->w, in->h));
	if (!m_graph)
		return false;
	auto src = m_src->outputs[0];
	auto frame = av_frame_alloc();
	mp_image_copy_fields_to_av_frame(frame, in);
	int flags = 0;
	if (!mp_image_is_writeable(in))
		flags |= AV_BUFFER_FLAG_READONLY;
	auto freeMpImage = [](void *in, uint8_t*) { talloc_free(in); };
	for (int n = 0; n < in->num_planes; ++n) {
		mp_image *plane = mp_image_new_ref(in);
		const size_t size = in->stride[n] * in->h;
		frame->buf[n] = av_buffer_create(in->planes[n], size, freeMpImage, plane, flags);
	}
	frame->pts = in->pts == MP_NOPTS_VALUE ? AV_NOPTS_VALUE : in->pts * av_q2d(av_inv_q(src->time_base));
	frame->sample_aspect_ratio = src->sample_aspect_ratio;
	const bool ok = (av_buffersrc_add_frame(m_src, frame) >= 0);
	av_frame_free(&frame);
	return ok;
}

mp_image *FFmpegFilterGraph::pull() {
	auto frame = av_frame_alloc();
	if (av_buffersink_get_frame(m_sink, frame) < 0) {
		av_frame_free(&frame);
		return nullptr;
	}
	auto freeAvFrame = [](void *frame) { av_frame_free((AVFrame**)&frame); };
	auto mpi = nullMpImage(frame, freeAvFrame);
	mp_image_copy_fields_from_av_frame(mpi, frame);
	return mpi;
}

bool FFmpegFilterGraph::linkGraph(AVFilterInOut *&in, AVFilterInOut *&out) {
	QString tmp;
#define	args (tmp.toLocal8Bit().constData())
	tmp.sprintf("width=%d:height=%d:pix_fmt=%d:time_base=1/%d:sar=1", m_size.width(), m_size.height(), imgfmt2pixfmt(m_imgfmt), AV_TIME_BASE);
	if (avfilter_graph_create_filter(&m_src, avfilter_get_by_name("buffer"), "src", args, nullptr, m_graph) < 0)
		return false;
	if (avfilter_graph_create_filter(&m_sink, avfilter_get_by_name("buffersink"), "sink", nullptr, nullptr, m_graph) < 0)
		return false;
	tmp = "pix_fmts=";
	for (int imgfmt = IMGFMT_START; imgfmt < IMGFMT_END; ++imgfmt) {
		if (!IMGFMT_IS_HWACCEL(imgfmt) && VideoOutput::queryFormat(nullptr, imgfmt)) {
			const char *name = av_get_pix_fmt_name(imgfmt2pixfmt(imgfmt));
			if (name) {
				tmp += name;
				tmp += '|';
			}
		}
	}
	tmp.chop(1);
	AVFilterContext *format = nullptr;
	if (avfilter_graph_create_filter(&format, avfilter_get_by_name("format"), "format", args, nullptr, m_graph) < 0)
		return false;
	if (avfilter_link(format, 0, m_sink, 0) < 0)
		return false;
	out->name = av_strdup("in");
	out->filter_ctx = m_src;
	in->name = av_strdup("out");
	in->filter_ctx = format;
	tmp.sprintf("flags=%d", SWS_BICUBIC);
	m_graph->scale_sws_opts = av_strdup(args);
	if (avfilter_graph_parse_ptr(m_graph, m_option.toLatin1().constData(), &in, &out, nullptr) < 0)
		return false;
	if (avfilter_graph_config(m_graph, nullptr) < 0)
		return false;
	Q_ASSERT(m_sink->nb_inputs == 1);
	Q_ASSERT(m_src->nb_outputs == 1);
#undef args
	return true;
}

bool FFmpegFilterGraph::initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt) {
	avfilter_register_all();
	if (option == m_option && m_size == size && m_imgfmt == imgfmt)
		return m_graph;
	m_option = option; m_size = size; m_imgfmt = imgfmt;
	release();
	if (option.isEmpty() || size.isEmpty() || imgfmt == IMGFMT_NONE || IMGFMT_IS_HWACCEL(imgfmt))
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


bool FFmpegPostProc::process(mp_image *dstImg, const mp_image *srcImg) const {
	if (!m_context)
		return false;
	const uint8_t *src[3]  = {srcImg->planes[0], srcImg->planes[1], srcImg->planes[2]};
		  uint8_t *dst[3]  = {dstImg->planes[0], dstImg->planes[1], dstImg->planes[2]};
	const int srcStride[3] = {srcImg->stride[0], srcImg->stride[1], srcImg->stride[2]};
	const int dstStride[3] = {dstImg->stride[0], dstImg->stride[1], dstImg->stride[2]};
	pp_postprocess(src, srcStride, dst, dstStride, srcImg->w, srcImg->h
				   , (const int8_t*)srcImg->qscale, srcImg->qstride, m_mode, m_context
				   , srcImg->pict_type | (srcImg->qscale_type ? PP_PICT_TYPE_QP2 : 0));
	return true;
}

bool FFmpegPostProc::initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt) {
	if (m_option == option && m_size == size && m_imgfmt == imgfmt)
		return m_context;
	release();
	m_option = option;
	m_size = size;
	m_imgfmt = imgfmt;
	if (m_option.isEmpty() || m_size.isEmpty())
		return false;
	int flags = 0;
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
	m_mode = pp_get_mode_by_name_and_quality(m_option.toLatin1().constData(), PP_QUALITY_MAX);
	if (!m_mode)
		return false;
	if (gCpuCaps.hasMMX)
		flags |= PP_CPU_CAPS_MMX;
	if (gCpuCaps.hasMMX2)
		flags |= PP_CPU_CAPS_MMX2;
	m_context = pp_get_context(size.width(), size.height(), flags);
	return m_context;
}

mp_image *FFmpegPostProc::newImage(const mp_image *mpi) const {
	auto tmp = mp_image_pool_get(m_pool, mpi->imgfmt, mpi->stride[0], mpi->h);
	auto img = mp_image_new_ref(tmp);
	talloc_free(tmp);
	img->w = mpi->w;
	img->h = mpi->h;
	img->stride[0] = mpi->stride[0];
	img->stride[1] = mpi->stride[1];
	img->stride[2] = mpi->stride[2];
	img->stride[3] = mpi->stride[3];
	mp_image_copy_attributes(img, (mp_image*)mpi);
	return img;
}
