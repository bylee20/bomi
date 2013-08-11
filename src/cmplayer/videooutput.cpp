#include "videooutput.hpp"
#include "videoframe.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"
#include "hwacc.hpp"
#include <mpvcore/mp_cmplayer.h>

extern "C" {
#include <video/out/vo.h>
#include <video/vfcap.h>
#include <video/decode/dec_video.h>
#include <video/img_fourcc.h>
#include <mpvcore/m_option.h>
#include <video/mp_image.h>
#include <sub/sub.h>
struct vo_driver video_out_null = VideoOutput::getDriver();
}

struct cmplayer_vo_priv { VideoOutput *vo; char *address; };

static VideoOutput *priv(struct vo *vo) { return static_cast<cmplayer_vo_priv*>(vo->priv)->vo; }

#define OPT_BASE_STRUCT struct cmplayer_vo_priv
const vo_driver &VideoOutput::getDriver() {
	static vo_driver driver;
	static bool first = true;
	if (first) {
		static m_option options[1];
		memset(options, 0, sizeof(options));
		options[0].name = "address";
		options[0].flags = 0;
		options[0].defval = 0;
		options[0].offset = MP_CHECKED_OFFSETOF(OPT_BASE_STRUCT, address, char*);
		options[0].____new = 1;
		options[0].type = &m_option_type_string;
		first = false;
		static vo_info_t info;
		info.name = "CMPlayer video output";
		info.short_name	= "null";
		info.author = "xylosper <darklin20@gmail.com>";
		info.comment = "";
		driver.info = &info;
		driver.preinit = preinit;
		driver.config = config;
		driver.control = control;
		driver.draw_osd = drawOsd;
		driver.flip_page = flipPage;
		driver.query_format = queryFormat;
		driver.draw_image = drawImage;
		driver.uninit = uninit;
		driver.options = options;
		driver.priv_size = sizeof(cmplayer_vo_priv);
	}
	return driver;
}

struct VideoOutput::Data {
	VideoFormat format;
	VideoFrame frame;
	mp_osd_res osd;
	PlayEngine *engine = nullptr;
	bool flip = false, quit = false;
	VideoRendererItem *renderer = nullptr;
	bool formatChanged = false;
	quint32 dest_w = 0, dest_h = 0;
	HwAcc *acc = nullptr;
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	memset(&d->osd, 0, sizeof(d->osd));
	d->engine = engine;
}

VideoOutput::~VideoOutput() {}

void VideoOutput::setHwAcc(HwAcc *acc) {
	d->acc = acc;
}

int VideoOutput::preinit(struct vo *vo) {
	auto priv = static_cast<cmplayer_vo_priv*>(vo->priv);
	priv->vo = (VideoOutput*)(void*)(quintptr)QString::fromLatin1(priv->address).toULongLong();
	return 0;
}

void VideoOutput::output(const QImage &image) {
	if (d->renderer)
		d->renderer->present(image);
}

void VideoOutput::setRenderer(VideoRendererItem *renderer) {
	d->renderer = renderer;
}

const VideoFormat &VideoOutput::format() const {
	return d->format;
}

int VideoOutput::config(struct vo *vo, uint32_t w_src, uint32_t h_src, uint32_t w_dest, uint32_t h_dest, uint32_t fs, uint32_t fmt) {
	Q_UNUSED(fs); Q_UNUSED(fmt); Q_UNUSED(w_src); Q_UNUSED(h_src);
	auto v = priv(vo); auto d = v->d;
	if (_Change(d->dest_w, w_dest))
		d->formatChanged = true;
	if (_Change(d->dest_h, h_dest))
		d->formatChanged = true;
	emit v->reconfigured();
	return 0;
}

void VideoOutput::drawImage(struct vo *vo, mp_image *mpi) {
	auto v = priv(vo); auto d = v->d;
	if (d->acc)
		mpi = d->acc->getFrame(mpi);
	if (d->formatChanged || (d->formatChanged = !d->format.compare(mpi)))
		emit v->formatChanged(d->format = VideoFormat(mpi, d->dest_w, d->dest_h));
	d->frame = VideoFrame(mpi, d->format);
	if (d->acc)
		mp_image_unrefp(&mpi);
	d->flip = true;
}

int VideoOutput::control(struct vo *vo, uint32_t req, void *data) {
	auto *v = priv(vo);
	switch (req) {
	case VOCTRL_REDRAW_FRAME:
		if (v->d->renderer)
			v->d->renderer->present(v->d->frame);
		return VO_TRUE;
	case VOCTRL_GET_HWDEC_INFO: {
		auto info = static_cast<mp_hwdec_info*>(data);
		info->vdpau_ctx = (mp_vdpau_ctx*)(void*)(v);
		return VO_TRUE;
	} default:
		return VO_NOTIMPL;
	}
}

void VideoOutput::drawOsd(struct vo *vo, struct osd_state *osd) {
	Data *d = priv(vo)->d;
	if (auto r = d->engine->videoRenderer()) {
		d->osd.w = d->format.width();
		d->osd.h = d->format.height();
		d->osd.display_par = 1.0;
		d->osd.video_par = vo->aspdat.par;
		static bool format[SUBBITMAP_COUNT] = {0, 0, 1, 0};
		osd_draw(osd, d->osd, osd->vo_pts, 0, format,  VideoRendererItem::drawMpOsd, r);
	}
}

void VideoOutput::flipPage(struct vo *vo) {
	Data *d = priv(vo)->d;
	if (!d->flip || d->quit)
		return;
	if (d->renderer) {
		d->renderer->present(d->frame, d->formatChanged);
		auto w = d->renderer->window();
		while (w && w->isVisible() && d->renderer->isFramePended() && !d->quit)
			PlayEngine::usleep(50);
	}
	d->flip = false;
}

void VideoOutput::quit() {
	d->quit = true;
}

int VideoOutput::queryFormat(struct vo */*vo*/, uint32_t format) {
	switch (format) {
	case IMGFMT_420P:	case IMGFMT_VAAPI:
	case IMGFMT_NV12:	case IMGFMT_NV21:
	case IMGFMT_YUYV:	case IMGFMT_UYVY:
	case IMGFMT_BGRA:	case IMGFMT_RGBA:
		return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_FLIP;
	default:
		return 0;
	}
}
