#include "videooutput.hpp"
#include "videoframe.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"
#include "hwacc.hpp"
#include "deintinfo.hpp"
#include "videofilter.hpp"
#include "mposditem.hpp"
#include "softwaredeinterlacer.hpp"
#include "vaapipostprocessor.hpp"
#include "log.hpp"
#include "mpv_helper.hpp"
extern "C" {
#include <video/hwdec.h>
#include <video/out/vo.h>
#include <video/vfcap.h>
#include <options/m_option.h>
#include <player/core.h>
#include <sub/sd.h>
#include <sub/osd_state.h>
}

struct cmplayer_vo_priv {
	VideoOutput *vo; char *address, *swdec_deint, *hwdec_deint;
};
static VideoOutput *priv(struct vo *vo) { return static_cast<cmplayer_vo_priv*>(vo->priv)->vo; }

DECLARE_LOG_CONTEXT(Video)

vo_driver create_driver() {
#define MPV_OPTION_BASE cmplayer_vo_priv
	static m_option options[] = {
		MPV_OPTION(address),
		MPV_OPTION(swdec_deint),
		MPV_OPTION(hwdec_deint),
		mpv::null_option
	};

	static vo_driver driver;
	driver.description = "CMPlayer video output";
	driver.name = "null";
	driver.buffer_frames = true;
	driver.preinit = VideoOutput::preinit;
	driver.reconfig = VideoOutput::reconfig;
	driver.control = VideoOutput::control;
	driver.draw_osd = VideoOutput::drawOsd;
	driver.flip_page = VideoOutput::flipPage;
	driver.query_format = VideoOutput::queryFormat;
	driver.get_buffered_frame = VideoOutput::getBufferedFrame;
	driver.draw_image = VideoOutput::drawImage;
	driver.uninit = VideoOutput::uninit;
	driver.options = options;
	driver.priv_size = sizeof(cmplayer_vo_priv);
	return driver;
}

vo_driver video_out_null = create_driver();

struct VideoOutput::Data {
	VideoFormat format;
	VideoFrame frame;
	mp_osd_res osd;
	PlayEngine *engine = nullptr;
	bool flip = false, deint = false;
	VideoFrame::Field upsideDown = VideoFrame::None;
	VideoRendererItem *renderer = nullptr;
	HwAcc *acc = nullptr, *prevAcc = nullptr;
	QLinkedList<VideoFrame> queue;
	mp_image_params params;
	DeintOption deint_swdec, deint_hwdec;
	SoftwareDeinterlacer deinterlacer;
	struct vo *vo = nullptr;
	QSize size, newSize;
	bool resized = false;
	QPoint mouse{-1, -1};
//	VaApiPostProcessor vaapi;
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	reset();
	d->engine = engine;
}

VideoOutput::~VideoOutput() {
	delete d;
}

void VideoOutput::setTargetSize(const QSize &size) {
	if (d->vo)
		d->newSize = size;
}

void VideoOutput::setFrameRect(const QRectF &rect) {
	if (d->vo)
		d->newSize = rect.size().toSize();
}

void VideoOutput::setHwAcc(HwAcc *acc) {
	d->acc = acc;
}

int VideoOutput::preinit(struct vo *vo) {
	auto priv = static_cast<cmplayer_vo_priv*>(vo->priv);
	priv->vo = address_cast<VideoOutput*>(priv->address);
	priv->vo->d->vo = vo;
	auto d = priv->vo->d;
	if (priv->swdec_deint)
		d->deint_swdec = DeintOption::fromString(priv->swdec_deint);
	if (priv->hwdec_deint)
		d->deint_hwdec = DeintOption::fromString(priv->hwdec_deint);
	priv->vo->updateDeint();
	return 0;
}

void VideoOutput::output(const QImage &image) {
	if (d->renderer)
		d->renderer->present(image);
}

void VideoOutput::setRenderer(VideoRendererItem *renderer) {
	if (_Change(d->renderer, renderer)) {
		if (d->renderer)
			connect(d->renderer->mpOsd(), &MpOsdItem::targetSizeChanged, this, &VideoOutput::setTargetSize);
		updateDeint();
	}
}

const VideoFormat &VideoOutput::format() const {
	return d->format;
}

void VideoOutput::updateDeint() {
	DeintOption opt;
	if (d->deint)
		opt = d->acc ? d->deint_hwdec : d->deint_swdec;
	d->deinterlacer.setOption(opt);
//	d->vaapi.setDeintOption(opt);
	if (d->renderer)
		d->renderer->setDeintMethod(opt.method);
}

void VideoOutput::reset() {
	memset(&d->params, 0, sizeof(d->params));
	memset(&d->osd, 0, sizeof(d->osd));
	d->params.colorlevels = MP_CSP_LEVELS_AUTO;
	d->params.colorspace = MP_CSP_AUTO;
	d->format = VideoFormat();
	d->frame = VideoFrame();
	d->queue.clear();
	d->flip = false;
}

int VideoOutput::reconfig(vo *out, mp_image_params *params, int flags) {
	auto v = priv(out); auto d = v->d;
	v->reset();
	d->upsideDown = (flags & VOFLAG_FLIPPING) ? VideoFrame::Flipped : VideoFrame::None;
	d->params = *params;
	return 0;
}

void VideoOutput::getBufferedFrame(struct vo *vo, bool /*eof*/) {
	auto v = priv(vo); auto d = v->d;
	vo->frame_loaded = !d->queue.isEmpty();
	if (vo->frame_loaded) {
		d->frame.swap(d->queue.first());
		d->queue.pop_front();
		vo->next_pts = d->frame.pts();
		if (!d->queue.isEmpty())
			vo->next_pts2 = d->queue.front().pts();
	}
	if (!d->frame.isNull() && _Change(d->format, d->frame.format()))
		emit v->formatChanged(d->format);
}

void VideoOutput::drawImage(struct vo *vo, mp_image *mpi) {
	auto v = priv(vo); auto d = v->d;
	if (_Change(d->prevAcc, d->acc))
		v->updateDeint();
	auto img = mpi;
	if (d->acc && d->acc->imgfmt() == mpi->imgfmt)
		img = d->acc->getImage(mpi);
	VideoFrame in(img != mpi, img, d->upsideDown);
//	if (IMGFMT_IS_VAAPI(mpi->imgfmt) && d->vaapi.apply(in, d->queue))
//		return;
	if (!d->deinterlacer.apply(in, d->queue))
		d->queue.push_back(in);
}

int VideoOutput::control(struct vo *vo, uint32_t req, void *data) {
	auto v = priv(vo); auto d = v->d;
	switch (req) {
	case VOCTRL_REDRAW_FRAME:
		d->renderer->rerender();
		return true;
	case VOCTRL_GET_HWDEC_INFO:
		static_cast<mp_hwdec_info*>(data)->vdpau_ctx = (mp_vdpau_ctx*)(void*)(v);
		return true;
	case VOCTRL_NEWFRAME:
		d->flip = true;
		return true;
	case VOCTRL_SKIPFRAME:
		d->flip = false;
		return true;
	case VOCTRL_RESET:
		v->reset();
		return true;
	case VOCTRL_GET_DEINTERLACE:
		*(int*)data = d->deint;
		return true;
	case VOCTRL_SET_DEINTERLACE:
		if (_Change(d->deint, (bool)*(int*)data))
			v->updateDeint();
		return true;
	case VOCTRL_WINDOW_TO_OSD_COORDS:
		return true;
	case VOCTRL_CHECK_EVENTS:
		if (d->renderer) {
			Q_ASSERT(vo->opts->enable_mouse_movements);
			if (_Change(d->mouse, d->renderer->mousePosition()))
				vo_mouse_movement(vo, d->mouse.x(), d->mouse.y());
		}
		if (_Change(d->size, d->newSize))
			vo->want_redraw = true;
		return true;
	default:
		return VO_NOTIMPL;
	}

}

void VideoOutput::drawOsd(struct vo *vo, struct osd_state *osd) {
	static const bool format[SUBBITMAP_COUNT] = {0, 1, 1, 1};
	static auto cb = [] (void *pctx, struct sub_bitmaps *imgs) {
		static_cast<MpOsdItem*>(pctx)->drawOn(imgs);
	};
	auto d = priv(vo)->d;
	if (auto r = d->renderer) {
		const auto dpr = r->devicePixelRatio();
		auto item = r->mpOsd();
		auto size = item->targetSize();
		d->osd.w = size.width();
		d->osd.h = size.height();
		d->osd.w *= dpr;
		d->osd.h *= dpr;
		d->osd.display_par = 1.0;
		item->setImageSize({d->osd.w, d->osd.h});
		osd_draw(osd, d->osd, osd->vo_pts, 0, format, cb, item);
	}
}

void VideoOutput::flipPage(struct vo *vo) {
	auto d = priv(vo)->d;
	if (!d->flip)
		return;
	d->flip = false;
	if (d->frame.isNull())
		return;
	if (d->renderer)
		d->renderer->present(d->frame);
	d->frame.clear();

}

int VideoOutput::queryFormat(struct vo */*vo*/, uint32_t format) {
	switch (format) {
	case IMGFMT_VDPAU:	case IMGFMT_VDA:	case IMGFMT_VAAPI:
	case IMGFMT_420P:	case IMGFMT_444P:
	case IMGFMT_420P16_LE:	case IMGFMT_420P16_BE:
	case IMGFMT_420P14_LE:	case IMGFMT_420P14_BE:
	case IMGFMT_420P12_LE:	case IMGFMT_420P12_BE:
	case IMGFMT_420P10_LE:	case IMGFMT_420P10_BE:
	case IMGFMT_420P9_LE:	case IMGFMT_420P9_BE:
	case IMGFMT_NV12:		case IMGFMT_NV21:
	case IMGFMT_YUYV:		case IMGFMT_UYVY:
	case IMGFMT_BGRA:		case IMGFMT_RGBA:
	case IMGFMT_ARGB:		case IMGFMT_ABGR:
	case IMGFMT_BGR0:		case IMGFMT_RGB0:
	case IMGFMT_0RGB:		case IMGFMT_0BGR:
		return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW;
	default:
		return 0;
	}
}

#ifdef Q_OS_LINUX
vo_driver video_out_vaapi;
vo_driver video_out_vdpau;
#endif
