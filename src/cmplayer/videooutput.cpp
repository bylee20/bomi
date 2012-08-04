#include "videooutput.hpp"
#include "overlay.hpp"
#include <QtCore/QDebug>
#include <QtCore/QSize>
#include <QtGui/QImage>
#include "videoframe.hpp"
#include "videorenderer.hpp"
#include "mpcore.hpp"

extern "C" {
#include <libvo/video_out.h>
#include <libmpcodecs/img_format.h>
#include <libmpcodecs/vfcap.h>
#include <libmpcodecs/mp_image.h>
#include <libvo/fastmemcpy.h>
#include <input/input.h>
#include <libvo/osd.h>
#include <sub/sub.h>
}

struct VideoOutput::Data {
	vo_driver driver;
	vo_info_t info;
	VideoFormat format;
	VideoRenderer *renderer;
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	memset(&d->info, 0, sizeof(d->info));
	memset(&d->driver, 0, sizeof(d->driver));

	d->info.name = "CMPlayer video output";
	d->info.short_name = "cmp";
	d->info.author = "xylosper <darklin20@gmail.com>";
	d->info.comment = "";

	d->driver.is_new = true,
	d->driver.info = &d->info;
	d->driver.preinit = preinit;
	d->driver.config = config;
	d->driver.control = control;
//	d->driver.draw_slice = draw_slice;
	d->driver.draw_osd = draw_osd;
	d->driver.flip_page = flip_page;
	d->driver.check_events = check_events;
	d->driver.uninit = uninit;

	d->renderer = new VideoRenderer(engine);
//	d->renderer->m_redraw = &d->driver.wants_redraw;
}

VideoOutput::~VideoOutput() {
	delete d->renderer;
	delete d;
}

VideoRenderer *VideoOutput::renderer() const {
	return d->renderer;
}

struct vo *VideoOutput::vo_create(MPContext *mpctx) {
	struct vo *vo = reinterpret_cast<struct vo*>(talloc_ptrtype(NULL, vo));
	memset(vo, 0, sizeof(*vo));
	vo->opts = &mpctx->opts;
	vo->x11 = mpctx->x11_state;
	vo->key_fifo = mpctx->key_fifo;
	vo->input_ctx = mpctx->input;
	vo->event_fd = -1;
	vo->registered_fd = -1;
	vo->priv = this;
	vo->driver = &d->driver;
	if (!vo->driver->preinit(vo, NULL))
		return vo;
	talloc_free_children(vo);
	free(vo);
	return nullptr;
}

int VideoOutput::preinit(struct vo */*vo*/, const char */*arg*/) {
	return 0;
}

void VideoOutput::uninit(struct vo */*vo*/) {
}

int VideoOutput::config(struct vo *vo, uint32_t w_s, uint32_t h_s, uint32_t, uint32_t, uint32_t, uint32_t fmt) {
	Data *d = reinterpret_cast<VideoOutput*>(vo->priv)->d;
	d->format.width = w_s;
	d->format.height = h_s;
	d->format.fourcc = fmt;
	return 0;
}

void VideoOutput::drawImage(void *data) {
	mp_image_t *mpi = reinterpret_cast<mp_image_t*>(data);
	VideoFrame &frame = d->renderer->bufferFrame();
	frame.format.bpp = mpi->bpp;
	switch (mpi->imgfmt) {
	case IMGFMT_I420:
		frame.format.fourcc = VideoFormat::I420;
		break;
	case IMGFMT_YV12:
		frame.format.fourcc = VideoFormat::YV12;
		break;
	default:
		return;
	}
	frame.format.stride = mpi->stride[0];
	frame.format.width = mpi->width;
	frame.format.height = mpi->height;

//	int length[4] = {mpi->stride[0]*mpi->height, mpi->stride[1]*mpi->height/2, mpi->stride[2]*mpi->height/2, 0};
//	frame.alloc(length);
//	fast_memcpy(frame.y(), mpi->planes[0], length[0]);
//	fast_memcpy(frame.u(), mpi->planes[1], length[1]);
//	fast_memcpy(frame.v(), mpi->planes[2], length[2]);
	frame.data[0] = mpi->planes[0];
	frame.data[1] = mpi->planes[1];
	frame.data[2] = mpi->planes[2];
//	frame.data[3] = mpi->planes[3];

	d->renderer->uploadBufferFrame();
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
		v->d->renderer->render();
		return VO_TRUE;
	case VOCTRL_FULLSCREEN:
		return VO_TRUE;
	case VOCTRL_UPDATE_SCREENINFO:
//		update_xinerama_info(vo);
		return VO_TRUE;
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

void VideoOutput::draw_osd(struct vo *vo, struct osd_state *osd) {
	Data *d = reinterpret_cast<VideoOutput*>(vo->priv)->d;
	osd_draw_text(osd, d->format.width, d->format.height, VideoRenderer::drawAlpha, d->renderer);
}

void VideoOutput::flip_page(struct vo *vo) {
	Data *d = reinterpret_cast<VideoOutput*>(vo->priv)->d;
	d->renderer->render();
}

void VideoOutput::check_events(struct vo */*vo*/) {}

int VideoOutput::queryFormat(int format) {
	if (format != IMGFMT_YV12 && format != IMGFMT_I420)
		return 0;
	return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW
		| VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN | VFCAP_ACCEPT_STRIDE | VOCAP_NOSLICES;
}

