#include "videooutput.hpp"
#include "videoframe.hpp"
#include "mpcore.hpp"
#include "hwaccel.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"

extern "C" {
#include <video/out/vo.h>
#include <video/vfcap.h>
#include <sub/sub.h>
#include <video/filter/vf.h>
#ifdef Q_OS_LINUX
extern vf_info_t vf_info_vo;
struct vf_priv_s { struct vo *vo; };
void prepare_hwaccel(sh_video *sh, AVCodecContext *avctx) {
	auto vf = sh->vfilter;
	while (vf) {
		if (vf->info == &vf_info_vo)
			break;
		vf = vf->next;
	}
	if (vf)
		static_cast<VideoOutput*>(vf->priv->vo->priv)->setAVCodecContext(avctx);
}
#endif
}


struct VideoOutput::Data {
	vo_driver driver;
	vo_info_t info;
	VideoFormat format;
#ifdef Q_OS_LINUX
	HwAccel hwAcc;
#endif
	mp_osd_res osd;
	mp_image_t *mpimg = nullptr;
	VideoFrame *frame = nullptr;
	PlayEngine *engine = nullptr;
	bool flip = false;
//	AVCodecContext *avctx = nullptr;
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	Q_ASSERT(VideoFormat::YV12 == IMGFMT_YV12);
	Q_ASSERT(VideoFormat::I420 == IMGFMT_I420);
	Q_ASSERT(VideoFormat::YUY2 == IMGFMT_YUY2);
	Q_ASSERT(VideoFormat::NV12 == IMGFMT_NV12);
	Q_ASSERT(VideoFormat::NV21 == IMGFMT_NV21);
	Q_ASSERT(VideoFormat::UYVY == IMGFMT_UYVY);

	memset(&d->info, 0, sizeof(d->info));
	memset(&d->driver, 0, sizeof(d->driver));
	memset(&d->osd, 0, sizeof(d->osd));

	d->info.name		= "CMPlayer video output";
	d->info.short_name	= "cmp";
	d->info.author		= "xylosper <darklin20@gmail.com>";
	d->info.comment		= "";

	d->driver.is_new = true,
	d->driver.info = &d->info;
	d->driver.preinit = preinit;
	d->driver.config = config;
	d->driver.control = control;
	d->driver.draw_slice = drawSlice;
	d->driver.draw_osd = drawOsd;
	d->driver.flip_page = flipPage;
	d->driver.check_events = checkEvents;
	d->driver.uninit = uninit;

	d->engine = engine;
}

VideoOutput::~VideoOutput() {
	d->hwAcc.finalize();
}

struct vo *VideoOutput::vo_create(MPContext *mpctx) {
	struct vo *vo = reinterpret_cast<struct vo*>(talloc_ptrtype(NULL, vo));
	memset(vo, 0, sizeof(*vo));
	vo->opts = &mpctx->opts;
	vo->key_fifo = mpctx->key_fifo;
	vo->input_ctx = mpctx->input;
	vo->event_fd = -1;
	vo->registered_fd = -1;
	vo->priv = this;
	vo->driver = &d->driver;
	if (!vo->driver->preinit(vo, NULL))
		return vo;
	talloc_free_children(vo);
	talloc_free(vo);
	return nullptr;
}

void VideoOutput::release() {

}

const VideoFormat &VideoOutput::format() const {
	return d->format;
}

bool VideoOutput::usingHwAccel() const {
	return d->hwAcc.isUsable();
}

void VideoOutput::setAVCodecContext(void *avctx) {
	 d->hwAcc.set(static_cast<AVCodecContext*>(avctx));
}

int VideoOutput::config(struct vo */*vo*/, uint32_t /*w_s*/, uint32_t /*h_s*/, uint32_t, uint32_t, uint32_t, uint32_t /*fmt*/) {
//	auto self = reinterpret_cast<VideoOutput*>(vo->priv);
//	Data *d = self->d;
	return 0;
}

bool VideoOutput::getImage(void *data) {
#ifdef Q_OS_MAC
	Q_UNUSED(data);
	return false;
#endif
#ifdef Q_OS_LINUX
	return d->hwAcc.isUsable() && d->hwAcc.setBuffer(static_cast<mp_image_t*>(data));
#endif
}

void VideoOutput::drawImage(void *data) {
	mp_image_t *mpi = reinterpret_cast<mp_image_t*>(data);
	if (auto renderer = d->engine->videoRenderer()) {
		VideoFrame &frame = renderer->getNextFrame();
		frame.setFormat(d->format);
#ifdef Q_OS_LINUX
		if (d->hwAcc.isUsable())
			mpi = &d->hwAcc.extract(mpi);
#endif
		if (frame.copy(mpi))
			emit formatChanged(d->format = frame.format());
#ifdef Q_OS_LINUX
		if (d->hwAcc.isUsable())
			d->hwAcc.clean();
#endif
	}
	d->flip = true;
}

int VideoOutput::control(struct vo *vo, uint32_t req, void *data) {
	VideoOutput *v = reinterpret_cast<VideoOutput*>(vo->priv);
	switch (req) {
	case VOCTRL_QUERY_FORMAT:
		return v->queryFormat(*reinterpret_cast<uint32_t*>(data));
	case VOCTRL_DRAW_IMAGE:
		v->drawImage(data);
		return VO_TRUE;
	case VOCTRL_REDRAW_FRAME:
		if (auto renderer = v->d->engine->videoRenderer())
			renderer->update();
		return VO_TRUE;
	case VOCTRL_FULLSCREEN:
		return VO_TRUE;
	case VOCTRL_UPDATE_SCREENINFO:
//		update_xinerama_info(vo);
		return VO_TRUE;
	case VOCTRL_GET_IMAGE:
		return v->getImage(data);
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

int VideoOutput::drawSlice(struct vo */*vo*/, uint8_t */*src*/[], int /*stride*/[], int w, int h, int x, int y) {
	qDebug() << "drawSlice();" << x << y << w << h;
	return true;
}

void VideoOutput::drawOsd(struct vo *vo, struct osd_state *osd) {
	Data *d = reinterpret_cast<VideoOutput*>(vo->priv)->d;
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
	Data *d = reinterpret_cast<VideoOutput*>(vo->priv)->d;
	if (!d->flip)
		return;
	if (auto renderer = d->engine->videoRenderer())
		renderer->next();
	d->flip = false;
}

void VideoOutput::checkEvents(struct vo */*vo*/) {}

int VideoOutput::queryFormat(int format) {
	switch (format) {
	case IMGFMT_I420:
	case IMGFMT_YV12:
	case IMGFMT_NV12:
	case IMGFMT_YUY2:
	case IMGFMT_UYVY:
#ifdef Q_OS_LINUX
	case IMGFMT_VDPAU:
#endif
		return VFCAP_OSD | VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_ACCEPT_STRIDE | VOCAP_NOSLICES;
	default:
		return 0;
	}
}

