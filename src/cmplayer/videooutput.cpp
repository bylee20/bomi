#include "videooutput.hpp"
#include "videoframe.hpp"
#include "mpcore.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"

extern "C" {
#include <video/out/vo.h>
#include <video/vfcap.h>
#include <video/img_fourcc.h>
#include <video/mp_image.h>
#include <sub/sub.h>
}


struct VideoOutput::Data {
	vo_driver driver;
	vo_info_t info;
	VideoFormat format;
	VideoFrame frame;
	mp_osd_res osd;
//	mp_image_t *mpimg = nullptr;
//	VideoFrame *frame = nullptr;
	PlayEngine *engine = nullptr;
	bool flip = false;
	bool hwAcc = false;
	VideoRendererItem *renderer = nullptr;
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	memset(&d->info, 0, sizeof(d->info));
	memset(&d->driver, 0, sizeof(d->driver));
	memset(&d->osd, 0, sizeof(d->osd));

	d->info.name		= "CMPlayer video output";
	d->info.short_name	= "cmp";
	d->info.author		= "xylosper <darklin20@gmail.com>";
	d->info.comment		= "";

	d->driver.info = &d->info;
	d->driver.preinit = preinit;
	d->driver.config = config;
	d->driver.control = control;
	d->driver.draw_osd = drawOsd;
	d->driver.flip_page = flipPage;
	d->driver.check_events = checkEvents;
	d->driver.query_format = queryFormat;
	d->driver.draw_image = drawImage;
	d->driver.uninit = uninit;

	d->engine = engine;
}

VideoOutput::~VideoOutput() {}

struct vo *VideoOutput::vo_create(MPContext *mpctx) {
	struct vo *svo = reinterpret_cast<struct vo*>(talloc_ptrtype(NULL, svo));
	memset(svo, 0, sizeof(*svo));
	svo->opts = &mpctx->opts;
	svo->key_fifo = mpctx->key_fifo;
	svo->input_ctx = mpctx->input;
	svo->event_fd = -1;
	svo->registered_fd = -1;
	svo->priv = this;
	svo->driver = &d->driver;
	if (!svo->driver->preinit(svo, NULL))
		return svo;
	talloc_free_children(svo);
	talloc_free(svo);
	return nullptr;
}

void VideoOutput::setRenderer(VideoRendererItem *renderer) {
	if (d->renderer)
		disconnect(d->renderer, 0, this, 0);
	connect(d->renderer = renderer, &VideoRendererItem::formatChanged, this, &VideoOutput::handleFormatChanged);
}

void VideoOutput::handleFormatChanged(const VideoFormat &format) {
	emit formatChanged(d->format = format);
}

const VideoFormat &VideoOutput::format() const {
	return d->format;
}

int VideoOutput::config(struct vo *vo, uint32_t /*w_s*/, uint32_t /*h_s*/, uint32_t, uint32_t, uint32_t, uint32_t /*fmt*/) {
	auto d = static_cast<VideoOutput*>(vo->priv)->d;
	d->hwAcc = false;//fmt == IMGFMT_VDPAU;
	return 0;
}

bool VideoOutput::getImage(void *data) {
	return false;
//	if (!d->hwAcc)
//		return false;
//	static_cast<mp_image_t*>(data)->flags |= MP_IMGFLAG_DIRECT;
//	return true;
//#ifdef Q_OS_MAC
//	Q_UNUSED(data);
//	return false;
//#endif
//	static_cast<mp_image_t*>(data)->flags |= MP_IMGFLAG_DIRECT;
//	return true;
//#ifdef Q_OS_LINUX
////	return d->hwAcc.isActivated() && d->hwAcc.setBuffer(static_cast<mp_image_t*>(data));
//#endif
}

//extern HwAcc *ha;

void VideoOutput::drawImage(struct vo *vo, mp_image *mpi) {
	auto v = static_cast<VideoOutput*>(vo->priv); auto d = v->d;
	d->frame = VideoFrame(mpi);
//	if (mpi->imgfmt == IMGFMT_VDPAU_FIRST)
//		mp_image_unrefp(&mpi);
//	if (auto renderer = d->engine->videoRenderer()) {
//		VideoFrame &frame = renderer->getNextFrame();
////		frame.setFormat(d->format);
////#ifdef Q_OS_LINUX
////		if (ha->isActivated())
////			mpi = &ha->extract(mpi);
////#endif
//		if (frame.copy(mpi))
//			emit v->formatChanged(d->format = frame.format());
//#ifdef Q_OS_LINUX
////		if (ha->isActivated())
////			ha->clean();
//#endif
//	}
	d->flip = true;
}

int VideoOutput::control(struct vo *vo, uint32_t req, void */*data*/) {
	VideoOutput *v = static_cast<VideoOutput*>(vo->priv);
	switch (req) {
	case VOCTRL_REDRAW_FRAME:
		if (v->d->renderer)
			v->d->renderer->present(v->d->frame);
		return VO_TRUE;
	case VOCTRL_FULLSCREEN:
	case VOCTRL_UPDATE_SCREENINFO:
	case VOCTRL_PAUSE:
	case VOCTRL_RESUME:
	case VOCTRL_GET_PANSCAN:
	case VOCTRL_SET_PANSCAN:
	case VOCTRL_SET_EQUALIZER:
	case VOCTRL_GET_EQUALIZER:
	case VOCTRL_SET_YUV_COLORSPACE:;
	case VOCTRL_GET_YUV_COLORSPACE:;
	case VOCTRL_ONTOP:
	default:
		return VO_NOTIMPL;
	}
}

void VideoOutput::drawOsd(struct vo *vo, struct osd_state *osd) {
	Data *d = static_cast<VideoOutput*>(vo->priv)->d;
	if (auto r = d->engine->videoRenderer()) {
		d->osd.w = d->format.width();
		d->osd.h = d->format.height();
		d->osd.display_par = 1.0;//vo->monitor_par;
		d->osd.video_par = vo->aspdat.par;
		static bool format[SUBBITMAP_COUNT] = {0, 0, 1, 1};
		osd_draw(osd, d->osd, osd->vo_pts, 0, format,  VideoRendererItem::drawMpOsd, r);
	}
}

void VideoOutput::flipPage(struct vo *vo) {
	Data *d = static_cast<VideoOutput*>(vo->priv)->d;
	if (!d->flip)
		return;
	if (d->renderer)
		d->renderer->present(d->frame);
//	if (auto renderer = d->engine->videoRenderer())
//		renderer->next();
	d->flip = false;
}

void VideoOutput::checkEvents(struct vo */*vo*/) {}

int VideoOutput::queryFormat(struct vo */*vo*/, uint32_t format) {
	switch (format) {
	case IMGFMT_420P:
	case IMGFMT_NV12:
	case IMGFMT_NV21:
	case IMGFMT_YUYV:
	case IMGFMT_UYVY:
#ifdef Q_OS_LINUX
	case IMGFMT_VDPAU_FIRST:
#endif
		return VFCAP_OSD | VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_FLIP;
	default:
		return 0;
	}
}

