#include "videooutput.hpp"
#include "videoframe.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"
#include "hwacc.hpp"
#include "deintinfo.hpp"
#include "videofilter.hpp"
#include "mposditem.hpp"

extern "C" {
#include <video/fmt-conversion.h>
#include <video/out/vo.h>
#include <video/filter/vf.h>
#include <video/vfcap.h>
#include <video/decode/dec_video.h>
#include <video/img_fourcc.h>
#include <video/mp_image_pool.h>
#include <mpvcore/m_option.h>
#include <video/mp_image.h>
#include <mpvcore/mp_core.h>
#include <sub/sub.h>
}


struct cmplayer_vo_priv { VideoOutput *vo; char *address; };

static VideoOutput *priv(struct vo *vo) { return static_cast<cmplayer_vo_priv*>(vo->priv)->vo; }

#define OPT_BASE_STRUCT struct cmplayer_vo_priv
vo_driver create_driver() {
	static m_option options[2];
	memset(options, 0, sizeof(options));
	options[0].name = "address";
	options[0].flags = 0;
	options[0].defval = 0;
	options[0].offset = MP_CHECKED_OFFSETOF(OPT_BASE_STRUCT, address, char*);
	options[0].is_new_option = 1;
	options[0].type = &m_option_type_string;

	static vo_info_t info;
	info.name = "CMPlayer video output";
	info.short_name	= "null";
	info.author = "xylosper <darklin20@gmail.com>";
	info.comment = "";

	static vo_driver driver;
	memset(&driver, 0, sizeof(driver));
	driver.info = &info;
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
	bool flip = false;
	int upsideDown = 0;
	VideoRendererItem *renderer = nullptr;
	HwAcc *acc = nullptr, *prevAcc = nullptr;
	QLinkedList<VideoFrame> frames;
	double prevPts = MP_NOPTS_VALUE;
	VideoFilter *pass = new PassThroughVideoFilter;
	mp_image_params params;
	DeintInfo deint_sw, deint_hw;
	DeintMode deintMode = DeintMode::Never;
	VideoFilter *filter_sw = nullptr;
	VideoFilter *filter_hw = nullptr;
	VideoFilter *filter = pass;
	template<typename T> T *newFilter(const QString &opts = QString()) {
		auto filter = new T; filter->setOptions(opts); return filter;
	}
	VideoFilter *makeFilter(const DeintInfo &deint) {
		const int flags = deint.flags();
		if (deint.isHardware())
			return newFilter<HardwareDeintFilter>(deint.isDoubleRate() ? "x2" : "");
		else if (flags & DeintInfo::PostProc) {
			QString options;
			switch (deint.method()) {
			case DeintInfo::LinearBob:
				options = "li";
				break;
			case DeintInfo::LinearBlend:
				options = "lb";
				break;
			case DeintInfo::CubicBob:
				options = "ci";
				break;
			case DeintInfo::Median:
				options = "md";
				break;
			default:
				break;
			}
			if (!options.isEmpty())
				return newFilter<FFmpegPostProcDeint>(deint.isDoubleRate() ? options + "x2" : options);
		} else if (flags & DeintInfo::AvFilter) {
			QString options;
			switch (deint.method()) {
			case DeintInfo::Yadif:
				options = "yadif";
				if (deint.isDoubleRate())
					options += "=mode=1";
				break;
			default:
				break;
			}
			if (!options.isEmpty())
				return newFilter<FFmpegAvFilter>(options);
		}
		return nullptr;
	}
};

VideoOutput::VideoOutput(PlayEngine *engine): d(new Data) {
	reset();
	d->engine = engine;
}

VideoOutput::~VideoOutput() {
	delete d->filter_sw;
	delete d->filter_hw;
	delete d->pass;
	delete d;
}

void VideoOutput::setHwAcc(HwAcc *acc) {
	d->acc = acc;
}

int VideoOutput::preinit(struct vo *vo) {
	auto priv = static_cast<cmplayer_vo_priv*>(vo->priv);
	priv->vo = address_cast<VideoOutput*>(priv->address);
	return 0;
}

void VideoOutput::output(const QImage &image) {
	if (d->renderer)
		d->renderer->present(image);
}

void VideoOutput::setRenderer(VideoRendererItem *renderer) {
	if (_Change(d->renderer, renderer))
		updateDeint();
}

const VideoFormat &VideoOutput::format() const {
	return d->format;
}

void VideoOutput::setDeintMode(DeintMode mode) {
	if (_Change(d->deintMode, mode))
		updateDeint();
}

void VideoOutput::updateDeint() {
	DeintInfo deint;
	auto deintAcc = [] (const DeintInfo &ref) {
		const int flags = ref.flags() & DeintInfo::Hardware;
		return flags ? DeintInfo(ref.method(), flags) : DeintInfo();
	};
	if (d->deintMode == DeintMode::Never) {
		deint = DeintInfo();
		d->filter = d->pass;
	} else {
		if (d->acc) {
			d->filter = d->filter_hw;
			deint = deintAcc(d->deint_hw);
		} else {
			d->filter = d->filter_sw;
			deint = deintAcc(d->deint_sw);
		}
	}
	if (!d->filter)
		d->filter = d->pass;
	if (d->renderer)
		d->renderer->setDeint(deint);
}

void VideoOutput::reset() {
	memset(&d->params, 0, sizeof(d->params));
	memset(&d->osd, 0, sizeof(d->osd));
	d->params.colorlevels = MP_CSP_LEVELS_AUTO;
	d->params.colorspace = MP_CSP_AUTO;
	d->format = VideoFormat();
	d->frame = VideoFrame();
	d->frames.clear();
	d->prevPts = MP_NOPTS_VALUE;
	d->flip = false;
	if (d->renderer)
		d->renderer->emptyQueue();
}

int VideoOutput::reconfig(vo *vo, mp_image_params *params, int flags) {
	auto v = priv(vo); auto d = v->d;
	d->upsideDown = (flags & VOFLAG_FLIPPING) ? VideoFrame::Flipped : 0;
	d->params = *params;
	v->reset();
	return 0;
}

HwAcc *VideoOutput::hwAcc() const {return d->acc;}

void VideoOutput::getBufferedFrame(struct vo *vo, bool /*eof*/) {
	auto v = priv(vo); auto d = v->d;
	vo->frame_loaded = !d->frames.isEmpty();
	if (vo->frame_loaded) {
		d->frame = d->frames.takeFirst();
		vo->next_pts = d->frame.pts();
		if (!d->frames.isEmpty())
			vo->next_pts2 = d->frames.front().pts();
	}
	if (_Change(d->format, d->frame.format()))
		emit v->formatChanged(d->format);
}

void VideoOutput::setDeint(const DeintInfo &sw, const DeintInfo &hw) {
	auto update = [this] (VideoFilter *&filter, DeintInfo &deint, const DeintInfo &newer) {
		if (!_Change(deint, newer)) return false;
		delete filter; filter = d->makeFilter(deint); return true;
	};
	bool changed = update(d->filter_sw, d->deint_sw, sw);
	changed = update(d->filter_hw, d->deint_hw, hw) || changed;
	if (changed)
		updateDeint();
}

void VideoOutput::drawImage(struct vo *vo, mp_image *mpi) {
	auto v = priv(vo); auto d = v->d;
	if (_Change(d->prevAcc, d->acc))
		v->updateDeint();
	auto img = mpi;
	if (d->acc && d->acc->imgfmt() == mpi->imgfmt)
		img = d->acc->getImage(mpi);
	const double pts = img->pts;
	auto filter = d->pass;
	if (d->deintMode == DeintMode::Always || (d->deintMode == DeintMode::Auto && (img->fields & MP_IMGFIELD_INTERLACED)))
		filter = d->filter;
	filter->setFrameFlags(d->upsideDown);
	filter->apply(img, d->frames, d->prevPts);
	d->prevPts = pts;
	if (img != mpi) // new image is allocated, unref it
		talloc_free(img);
}

int VideoOutput::control(struct vo *vo, uint32_t req, void *data) {
	auto v = priv(vo); auto d = v->d;
	switch (req) {
	case VOCTRL_REDRAW_FRAME:
		if (d->renderer)
			d->renderer->present(d->frame);
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
	default:
		return VO_NOTIMPL;
	}
}

void VideoOutput::drawOsd(struct vo *vo, struct osd_state *osd) {
	Data *d = priv(vo)->d;
	static auto cb = [] (void *pctx, struct sub_bitmaps *imgs) {
		static_cast<MpOsdItem*>(pctx)->drawOn(imgs);
	};
	if (auto r = d->engine->videoRenderer()) {
		auto item = r->mpOsd();
		auto size = item->targetSize();
		d->osd.w = size.width();
		d->osd.h = size.height();
		item->setRenderSize(size);
		d->osd.display_par = vo->aspdat.monitor_par;
		d->osd.video_par = vo->aspdat.par;
		static const bool format[SUBBITMAP_COUNT] = {0, 1, 1, 1};
		osd_draw(osd, d->osd, osd->vo_pts, 0, format, cb, item);
	}
}

void VideoOutput::flipPage(struct vo *vo) {
	Data *d = priv(vo)->d;
	if (!d->flip)
		return;
	if (d->renderer)
		d->renderer->present(d->frame);
	d->flip = false;
}

int VideoOutput::queryFormat(struct vo */*vo*/, uint32_t format) {
	switch (format) {
	case IMGFMT_VDPAU:	case IMGFMT_VDA:	case IMGFMT_VAAPI:
	case IMGFMT_420P:
	case IMGFMT_420P16_LE:	case IMGFMT_420P16_BE:
	case IMGFMT_420P14_LE:	case IMGFMT_420P14_BE:
	case IMGFMT_420P12_LE:	case IMGFMT_420P12_BE:
	case IMGFMT_420P10_LE:	case IMGFMT_420P10_BE:
	case IMGFMT_420P9_LE:	case IMGFMT_420P9_BE:
	case IMGFMT_NV12:		case IMGFMT_NV21:
	case IMGFMT_YUYV:		case IMGFMT_UYVY:
	case IMGFMT_BGRA:		case IMGFMT_RGBA:
		return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_FLIP;
	default:
		return 0;
	}
}

#ifdef Q_OS_LINUX
vo_driver video_out_vaapi;
vo_driver video_out_vdpau;
#endif
