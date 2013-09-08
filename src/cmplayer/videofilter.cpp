#include "videofilter.hpp"
#include "hwacc.hpp"
#include "videooutput.hpp"

bool VideoFilter::pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double /*prevPts*/) {
	queue.push_back(VideoFrame(mpi, frameFlags()));
	return true;
}

bool VideoFilter::build() { return true; }

bool FFmpegAvFilter::pass(mp_image *img, QLinkedList<VideoFrame> &queue, double prevPts) {
	if (!m_graph)
		return false;
	mp_image *mpi = mp_image_new_ref(img);
	auto colorspace = mpi->colorspace;
	auto colorrange = mpi->levels;
	auto src = m_src->outputs[0];
	auto frame = av_frame_alloc();
	mp_image_copy_fields_to_av_frame(frame, mpi);
	int flags = 0;
	if (!mp_image_is_writeable(mpi))
		flags |= AV_BUFFER_FLAG_READONLY;
	for (int i=0; i<mpi->num_planes; ++i)
		frame->data[i] = mpi->planes[i] + mpi->stride[i];
	auto freeMpImage = [](void *mpi, uint8_t*) {talloc_free(mpi);};
	for (int n = 0; n < mpi->num_planes; ++n) {
		struct mp_image *plane = mp_image_new_ref(mpi);
		size_t size = mpi->stride[n] * mpi->h;
		frame->buf[n] = av_buffer_create(mpi->planes[n], size, freeMpImage, plane, flags);
	}
	frame->pts = mpi->pts == MP_NOPTS_VALUE ? AV_NOPTS_VALUE : mpi->pts * av_q2d(av_inv_q(src->time_base));
	frame->sample_aspect_ratio = src->sample_aspect_ratio;
	const bool ok = (av_buffersrc_add_frame(m_src, frame) >= 0);
	av_frame_free(&frame);
	talloc_free(mpi);
	if (!ok)
		return false;
	auto freeAvFrame = [](void *frame) {av_frame_free((AVFrame**)&frame);};
	QLinkedList<mp_image*> images;
	for (int i=0;;++i) {
		frame = av_frame_alloc();
		if (av_buffersink_get_frame(m_sink, frame) < 0) {
			av_frame_free(&frame);
			break;
		}
		auto got = nullMpImage(frame, freeAvFrame);
		mp_image_copy_fields_from_av_frame(got, frame);
		mp_image_set_display_size(got, img->display_w, img->display_h);
		got->pts = img->pts;
		got->colorspace = colorspace;
		got->levels = colorrange;
		images.append(got);
	}
	if (images.isEmpty())
		queue.push_back(VideoFrame(img, frameFlags()));
	else {
		auto it = images.begin();
		queue.push_back(VideoFrame(*it, frameFlags()));
		talloc_free(*it);
		for (++it; it != images.end(); ++it) {
			auto mpi = *it;
			mpi->pts = queue.back().nextPts(prevPts, images.size());
			queue.push_back(VideoFrame(mpi, frameFlags()));
			talloc_free(mpi);
		}
	}
	return ok;
}

bool FFmpegAvFilter::build() {
	avfilter_register_all();
	release();
	if (format().isEmpty())
		return false;
	bool ok = false;
	auto out = avfilter_inout_alloc();
	auto in = avfilter_inout_alloc();
	m_graph = avfilter_graph_alloc();
	do {
		if (!out || !in || !m_graph)
			break;
		QString tmp;
#define	args (tmp.toLocal8Bit().constData())
		tmp.sprintf("width=%d:height=%d:pix_fmt=%d:time_base=1/%d:sar=1", format().width(), format().height(), imgfmt2pixfmt(format().type()), AV_TIME_BASE);
		if (avfilter_graph_create_filter(&m_src, avfilter_get_by_name("buffer"), "src", args, nullptr, m_graph) < 0)
			break;
		if (avfilter_graph_create_filter(&m_sink, avfilter_get_by_name("buffersink"), "sink", nullptr, nullptr, m_graph) < 0)
			break;
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
			break;
		if (avfilter_link(format, 0, m_sink, 0) < 0)
			break;

		out->name = av_strdup("in");
		out->filter_ctx = m_src;
		in->name = av_strdup("out");
		in->filter_ctx = format;
		tmp.sprintf("flags=%d", SWS_BICUBIC);
		m_graph->scale_sws_opts = av_strdup(args);
		if (avfilter_graph_parse_ptr(m_graph, options().toLatin1().constData(), &in, &out, nullptr) < 0)
			break;
		if (avfilter_graph_config(m_graph, nullptr) < 0)
			break;
		ok = true;
		Q_ASSERT(m_sink->nb_inputs == 1);
		Q_ASSERT(m_src->nb_outputs == 1);
#undef args
	} while (false);
	if (!ok)
		release();
	avfilter_inout_free(&out);
	avfilter_inout_free(&in);
	return ok;
}

mp_image *FFmpegPostProcDeint::newImage(const mp_image *mpi) const {
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

void FFmpegPostProcDeint::process(mp_image *dstImg, const mp_image *srcImg) const {
	const uint8_t *src[3] = {srcImg->planes[0], srcImg->planes[1], srcImg->planes[2]};
	const int srcStride[3] = {srcImg->stride[0], srcImg->stride[1], srcImg->stride[2]};
	uint8_t *dst[3] = {dstImg->planes[0], dstImg->planes[1], dstImg->planes[2]};
	const int dstStride[3] = {dstImg->stride[0], dstImg->stride[1], dstImg->stride[2]};
	pp_postprocess(src, srcStride, dst, dstStride, srcImg->w, srcImg->h
				   , (const int8_t*)srcImg->qscale, srcImg->qstride, m_mode, m_context
				   , srcImg->pict_type | (srcImg->qscale_type ? PP_PICT_TYPE_QP2 : 0));
}

void FFmpegPostProcDeint::paintOut(mp_image *dst, const mp_image *src) const {
	auto copy = [dst, src] (int y) {
//		memcpy(img->planes[0] + y*img->stride[0], mpi->planes[0] + y*mpi->stride[0], mpi->stride[0]);
		memset(dst->planes[0] + y*dst->stride[0], 0, dst->stride[0]);
	};
	copy(0);
	copy(src->h-1);
}

VideoFrame FFmpegPostProcDeint::topField(mp_image *mpi) const {
	auto img = newImage(mpi);
	process(img, mpi);
	paintOut(img, mpi);
	const VideoFrame frame(img, format(), frameFlags());
	talloc_free(img);
	return frame;
}

VideoFrame FFmpegPostProcDeint::bottomField(mp_image *mpi) const {
	auto img = newImage(mpi);
	mpi->planes[0] += mpi->stride[0];
	img->planes[0] += img->stride[0];
	mpi->h -= 2;
	img->h -= 2;
	process(img, mpi);
	img->planes[0] -= img->stride[0];
	mpi->planes[0] -= mpi->stride[0];
	img->h += 2;
	mpi->h += 2;
	paintOut(img, mpi);
	const VideoFrame frame(img, format(), frameFlags());
	talloc_free(img);
	return frame;
}

bool FFmpegPostProcDeint::pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) {
	if (!m_context || !m_mode) {
		queue.push_back(VideoFrame(mpi, format()));
		return false;
	}
	Q_ASSERT(mpi->stride[0] >= ((mpi->w+7)&(~7)));
	const bool topFirst = mpi->fields & MP_IMGFIELD_TOP_FIRST;
	queue.push_back(topFirst ? topField(mpi) : bottomField(mpi));
	if (m_doubler) {
		mpi->pts = queue.last().nextPts(prevPts);
		queue.push_back(topFirst ? bottomField(mpi) : topField(mpi));
	}
	return true;
}

bool FFmpegPostProcDeint::build() {
	release();
	if (format().isEmpty() || format().isNative())
		return false;
	auto options = this->options();
	if ((m_doubler = options.endsWith("x2")))
		options.chop(2);
	m_mode = pp_get_mode_by_name_and_quality(options.toLatin1().constData(), PP_QUALITY_MAX);
	if (!m_mode)
		return false;
	int flags = 0;
	if (gCpuCaps.hasMMX)
		flags |= PP_CPU_CAPS_MMX;
	if (gCpuCaps.hasMMX2)
		flags |= PP_CPU_CAPS_MMX2;
	switch (format().type()) {
	case IMGFMT_444P: flags|= PP_FORMAT_444; break;
	case IMGFMT_422P: flags|= PP_FORMAT_422; break;
	case IMGFMT_411P: flags|= PP_FORMAT_411; break;
	default:          flags|= PP_FORMAT_420; break;
	}
	m_context = pp_get_context(format().width(), format().height(), flags);
	return m_context;
}

bool HardwareDeintFilter::pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) {
	static const VideoFrame::Field field[] = {VideoFrame::Bottom, VideoFrame::Top};
	const bool topFirst = mpi->fields & MP_IMGFIELD_TOP_FIRST;
	queue.append(VideoFrame(mpi, field[topFirst] | frameFlags()));
	if (m_doubler)
		queue.append(VideoFrame(mpi, queue.last().nextPts(prevPts), field[!topFirst] | VideoFrame::Additional | frameFlags()));
	return true;
}

bool HardwareDeintFilter::build() {
	m_doubler = options().contains("x2");
	return true;
}
