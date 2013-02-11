extern "C" {
#include "video/fmt-conversion.h"
#include "video/filter/vf.h"
#include "video/decode/vd.h"
#include "video/img_format.h"
#include "demux/demux_packet.h"
#include "core/codec-cfg.h"
}
#include "stdafx.hpp"
#include <va/va.h>
#include <va/va_x11.h>
#include <libavcodec/vaapi.h>
#include <libavcodec/avcodec.h>
#include "mpv-vaapi.hpp"

//static void draw_slice(struct AVCodecContext *s, const AVFrame *src, int offset[4], int y, int type, int height);
struct VaApiCodec { AVCodecID id = AV_CODEC_ID_NONE; QVector<VAProfile> profiles; int surfaceCount = 0; };
struct VaApiInfo::Data { QMap<AVCodecID, VaApiCodec> supported; Display *xdpy = nullptr; VADisplay display = nullptr; };

VaApiInfo::VaApiInfo(): d(new Data) {
	if (!(d->xdpy = XOpenDisplay(NULL)) || !(d->display = vaGetDisplay(d->xdpy)))
		return;
	int major, minor;
	if (vaInitialize(d->display, &major, &minor) != VA_STATUS_SUCCESS)
		return;
	auto size = vaMaxNumProfiles(d->display);
	QVector<VAProfile> profiles;
	profiles.resize(size);
	if (vaQueryConfigProfiles(d->display, profiles.data(), &size) != VA_STATUS_SUCCESS)
		return;
	profiles.resize(size);
	auto supports = [this, &profiles](const QVector<VAProfile> &candidates, int count, AVCodecID id) {
		VaApiCodec codec;
		for (auto one : candidates) {
			if (profiles.contains(one))
				codec.profiles.push_back(one);
		}
		if (!codec.profiles.isEmpty()) {
			codec.surfaceCount = count;
			codec.id = id;
		}
		return codec;
	};
#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */
	const QVector<VAProfile> mpeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
	const QVector<VAProfile> mpeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
	const QVector<VAProfile> h264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};
	const QVector<VAProfile> wmv3s = {VAProfileVC1Main, VAProfileVC1Simple};
	const QVector<VAProfile> vc1s = {VAProfileVC1Advanced};
	d->supported[AV_CODEC_ID_MPEG1VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG1VIDEO);
	d->supported[AV_CODEC_ID_MPEG2VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG2VIDEO);
	d->supported[AV_CODEC_ID_MPEG4] = supports(mpeg4s, NUM_VIDEO_SURFACES_MPEG4, AV_CODEC_ID_MPEG4);
	d->supported[AV_CODEC_ID_WMV3] = supports(wmv3s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_WMV3);
	d->supported[AV_CODEC_ID_VC1] = supports(vc1s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_VC1);
	d->supported[AV_CODEC_ID_H264] = supports(h264s, NUM_VIDEO_SURFACES_H264, AV_CODEC_ID_H264);
}
VaApiInfo &VaApiInfo::get() { static VaApiInfo info; return info; }
VaApiInfo::~VaApiInfo() { delete d; }
const VaApiCodec *VaApiInfo::find(AVCodecID id) { const auto &i = get(); auto it = i.d->supported.find(id); return it != i.d->supported.end() && !it->profiles.isEmpty() ? &(*it) : 0; }
void *VaApiInfo::display() { return get().d->display; }
void VaApiInfo::finalize() { auto &i = get(); if (i.d->display) vaTerminate(i.d->display); if (i.d->xdpy) XCloseDisplay(i.d->xdpy); }


struct VaApi {
	struct VaApiSurface { VASurfaceID  id = VA_INVALID_ID; bool ref = false; quint64 order = 0; };
	VaApi(sh_video *sh);
	~VaApi();
	static int init(sh_video_t *sh) { VaApi *vaapi = new VaApi(sh); return vaapi->m_ok ? 1 : ((delete vaapi), 0); }
	static void uninit(sh_video_t *sh) { delete static_cast<VaApi*>(sh->context); }
	static int get_buffer(AVCodecContext *avctx, AVFrame *pic) { return static_cast<VaApi*>(avctx->opaque)->getBuffer(pic); }
	static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic) { static_cast<VaApi*>(avctx->opaque)->releaseBuffer(pic); }
	static mp_image *decode(sh_video *sh, demux_packet *packet, void *data, int len, int flags, double *reordered_pts) {
		return static_cast<VaApi*>(sh->context)->decode(packet, data, len, flags, reordered_pts);
	}
	mp_image *decode(demux_packet *packet, void *data, int len, int flags, double *reordered_pts);
	bool initVideoOutput(AVPixelFormat pix_fmt);
	int getBuffer(AVFrame *pic);
	void releaseBuffer(AVFrame *pic);
	static PixelFormat find(AVCodecContext *avctx, const PixelFormat *fmt) {
		auto vaapi = static_cast<VaApi*>(avctx->opaque);
		for (int i = 0; fmt[i] != PIX_FMT_NONE; i++) {
			if (fmt[i] == PIX_FMT_VAAPI_VLD && vaapi->initVideoOutput(fmt[i]))
				return fmt[i];
		}
		return PIX_FMT_NONE;
	}
	void freeContext();
	bool fillContext();
	void cleanImage() {
		if (m_vaImage.image_id != VA_INVALID_ID) {
			VAStatus status;
			if ((status = vaUnmapBuffer(m_context.display, m_vaImage.buf)) != VA_STATUS_SUCCESS)
				qDebug() << "va(UnmapBuffer)():" << vaErrorStr(status);
			vaDestroyImage(m_context.display, m_vaImage.image_id);
			m_vaImage.image_id = VA_INVALID_ID;
		}
	}

//private:

	AVCodecContext *m_avctx = nullptr;
	AVFrame *m_pic = nullptr;
	bool m_vo = false;
	AVRational m_last_sample_aspect_ratio;
	sh_video *m_sh = nullptr;
	bool m_ok = false;
	vaapi_context m_context;
	QVector<VaApiSurface> m_surfaces;
	quint64 m_surfaceOrder = 0;
	VAProfile m_profile = (VAProfile)(-1);
	VAImage m_vaImage;
};

VaApi::VaApi(sh_video *sh) {
	memset(&m_context, 0, sizeof(m_context));
	m_context.config_id = m_context.context_id = m_vaImage.image_id = VA_INVALID_ID;
	m_context.display = VaApiInfo::display();
	(m_sh = sh)->context = this;
	AVCodec *codec = nullptr;
	if (sh->codec->dll)
		codec = avcodec_find_decoder_by_name(sh->codec->dll);
	else if (sh->libav_codec_id)
		codec = avcodec_find_decoder((AVCodecID)sh->libav_codec_id);
	if (!codec)
		return;
	sh->codecname = codec->long_name ? codec->long_name : codec->name;
	m_pic = avcodec_alloc_frame();
	m_avctx = avcodec_alloc_context3(codec);
	m_avctx->opaque = this;
	m_avctx->codec_type = AVMEDIA_TYPE_VIDEO;
	m_avctx->codec_id = codec->id;
	m_avctx->get_format = find;
	m_avctx->get_buffer = get_buffer;
	m_avctx->release_buffer = release_buffer;
	m_avctx->reget_buffer = get_buffer;
//	m_avctx->draw_horiz_band = draw_slice;
	m_avctx->slice_flags = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
	m_avctx->flags |= CODEC_FLAG_EMU_EDGE;
	m_avctx->coded_width = sh->disp_w;
	m_avctx->coded_height = sh->disp_h;
	m_avctx->codec_tag = sh->format;
	if (sh->gsh->lavf_codec_tag)
		m_avctx->codec_tag = sh->gsh->lavf_codec_tag;
	m_avctx->stream_codec_tag = sh->video.fccHandler;
	if (sh->bih && sh->bih->biSize > (int)sizeof(*sh->bih)) {
		m_avctx->extradata_size = sh->bih->biSize - sizeof(*sh->bih);
		m_avctx->extradata = (uchar*)av_mallocz(m_avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
		memcpy(m_avctx->extradata, sh->bih + 1, m_avctx->extradata_size);
	}
	if (sh->bih)
		m_avctx->bits_per_coded_sample = sh->bih->biBitCount;
	m_avctx->thread_count = 1;
	if (avcodec_open2(m_avctx, codec, nullptr) < 0)
		return;
	m_ok = true;
}

VaApi::~VaApi() {
	freeContext();
	m_sh->codecname = NULL;
	if (m_avctx) {
		if (m_avctx->codec && avcodec_close(m_avctx) < 0)
			mp_tmsg(MSGT_DECVIDEO, MSGL_ERR, "Could not close codec.\n");
		av_freep(&m_avctx->extradata);
		av_freep(&m_avctx->slice_offset);
	}
	av_freep(&m_avctx);
	avcodec_free_frame(&m_pic);
	m_sh->context = nullptr;
}

void VaApi::freeContext() {
	cleanImage();
	if (m_context.display) {
		if (m_context.context_id != VA_INVALID_ID)
			vaDestroyContext(m_context.display, m_context.context_id);
		for (auto &surface : m_surfaces) {
			if (surface.id != VA_INVALID_SURFACE)
				vaDestroySurfaces(m_context.display, &surface.id, 1);
		}
		if (m_context.config_id != VA_INVALID_ID)
			vaDestroyConfig(m_context.display, m_context.config_id);
	}
	m_context.context_id = m_context.config_id = VA_INVALID_ID;
	m_profile = (VAProfile)(-1);
	m_surfaces.clear();
	m_surfaceOrder = 0;
}

bool VaApi::fillContext() {
	m_avctx->hwaccel_context = nullptr;
	auto codec = VaApiInfo::find(m_avctx->codec_id);
	if (!codec)
		return false;
	freeContext();
	m_profile = codec->profiles.front();
	VAStatus status = VA_STATUS_SUCCESS;
	do {
		VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
		if((status = vaGetConfigAttributes(m_context.display, m_profile, VAEntrypointVLD, &attr, 1)) != VA_STATUS_SUCCESS)
			break;
		if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
			{ status = VA_STATUS_ERROR_FLAG_NOT_SUPPORTED; break; }
		if((status = vaCreateConfig(m_context.display, m_profile, VAEntrypointVLD, &attr, 1, &m_context.config_id)) != VA_STATUS_SUCCESS)
			break;
		QVector<VASurfaceID> ids(codec->surfaceCount, VA_INVALID_SURFACE);
		if ((status = vaCreateSurfaces(m_context.display, m_avctx->width, m_avctx->height, VA_RT_FORMAT_YUV420, ids.size(), ids.data())) != VA_STATUS_SUCCESS
				&& (status = vaCreateSurfaces(m_context.display, m_avctx->width, m_avctx->height, VA_RT_FORMAT_YUV422, ids.size(), ids.data())) != VA_STATUS_SUCCESS)
			break;
		m_surfaces.resize(ids.size());
		for (int i=0; i<ids.size(); ++i)
			m_surfaces[i].id = ids[i];
		if ((status = vaCreateContext(m_context.display, m_context.config_id, m_avctx->width, m_avctx->height, VA_PROGRESSIVE, ids.data(), ids.size(), &m_context.context_id)) != VA_STATUS_SUCCESS)
			break;
		VAImage vaImage;
		if ((status = vaDeriveImage(m_context.display, ids[0], &vaImage)) != VA_STATUS_SUCCESS)
			break;
		vaDestroyImage(m_context.display, vaImage.image_id);
		m_avctx->hwaccel_context = &m_context;
		return true;
	} while(0);
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "vaapi:" << vaErrorStr(status);
	return false;
}

bool VaApi::initVideoOutput(PixelFormat pixfmt) {
	if (pixfmt != AV_PIX_FMT_VAAPI_VLD)
		return false;
	if (av_cmp_q(m_avctx->sample_aspect_ratio, m_last_sample_aspect_ratio) ||
			m_avctx->width != m_sh->disp_w || m_avctx->height != m_sh->disp_h || !m_vo) {
		m_vo = false;
		if (m_sh->aspect == 0 || m_last_sample_aspect_ratio.den)
			m_sh->aspect = av_q2d(m_avctx->sample_aspect_ratio)*m_avctx->width/m_avctx->height;
		m_last_sample_aspect_ratio = m_avctx->sample_aspect_ratio;
		m_sh->disp_w = m_avctx->width;
		m_sh->disp_h = m_avctx->height;
		m_sh->colorspace = avcol_spc_to_mp_csp(m_avctx->colorspace);
		m_sh->color_range = avcol_range_to_mp_csp_levels(m_avctx->color_range);
		if (!fillContext() || !mpcodecs_config_vo(m_sh, m_sh->disp_w, m_sh->disp_h, IMGFMT_VDPAU_FIRST))
			return false;
		m_vo = true;
	}
	return true;
}

int VaApi::getBuffer(AVFrame *pic) {
	if (!initVideoOutput(m_avctx->pix_fmt))
		return -1;
	int i_old, i;
	for (i=0, i_old=0; i<m_surfaces.size(); ++i) {
		if (!m_surfaces[i].ref)
			break;
		if (m_surfaces[i].order < m_surfaces[i_old].order)
			i_old = i;
	}
	if (i >= m_surfaces.size())
		i = i_old;
	m_surfaces[i].ref = true;
	m_surfaces[i].order = ++m_surfaceOrder;
	Q_ASSERT(m_surfaces[i].id != VA_INVALID_ID);
	uchar *dummy = (uchar*)(uintptr_t)m_surfaces[i].id;
	pic->linesize[0] = pic->linesize[1] = pic->linesize[2] = pic->linesize[3] = 0;
	pic->data[1] = pic->data[2] = nullptr;
	pic->data[0] = pic->data[3] = dummy;
	pic->type = FF_BUFFER_TYPE_USER;
	pic->reordered_opaque = m_avctx->reordered_opaque;
	return 0;
}

void VaApi::releaseBuffer(AVFrame *pic) {
	const auto id = (VASurfaceID)(uintptr_t)pic->data[3];
	for (int i=0; i<m_surfaces.size(); ++i) {
		if (m_surfaces[i].id == id) {
			m_surfaces[i].ref = false;
			break;
		}
	}
	pic->data[0] = pic->data[1] = pic->data[2] = pic->data[3] = nullptr;
}

union pts { int64_t i; double d; };
mp_image *VaApi::decode(demux_packet *packet, void *data, int len, int flags,  double *reordered_pts) {
	if (flags & 2)
		m_avctx->skip_frame = AVDISCARD_ALL;
	else if (flags & 1)
		m_avctx->skip_frame = AVDISCARD_NONREF;
	else
		m_avctx->skip_frame = AVDISCARD_DEFAULT;

	AVPacket pkt; av_init_packet(&pkt);
	pkt.data = (uchar*)data; pkt.size = len;
	if (packet && packet->keyframe)
		pkt.flags |= AV_PKT_FLAG_KEY;
	if (packet && packet->avpacket) {
		pkt.side_data = packet->avpacket->side_data;
		pkt.side_data_elems = packet->avpacket->side_data_elems;
	}

	// The avcodec opaque field stupidly supports only int64_t type
	pts temp; temp.d = *reordered_pts;
	m_avctx->reordered_opaque = temp.i;
	int newFrame = 0;
	if (avcodec_decode_video2(m_avctx, m_pic, &newFrame, &pkt) < 0)
		mp_msg(MSGT_DECVIDEO, MSGL_WARN, "Error while decoding frame!\n");
	temp.i = m_pic->reordered_opaque; *reordered_pts = temp.d;

	if (!newFrame)
		return nullptr;
	if (!initVideoOutput(m_avctx->pix_fmt))
		return nullptr;
	const auto id = (VASurfaceID)(uintptr_t)m_pic->data[3];
	vaSyncSurface(m_context.display, id);
	VAStatus status;
	mp_image_t *mpi = nullptr;
	do {
		if ((status = vaDeriveImage(m_context.display, id, &m_vaImage)) != VA_STATUS_SUCCESS)
			break;
		uchar *temp = nullptr;
		if ((status = vaMapBuffer(m_context.display, m_vaImage.buf, (void**)&temp)) != VA_STATUS_SUCCESS)
			break;
		quint32 imgfmt = 0;
		switch (m_vaImage.format.fourcc) {
		case VA_FOURCC_YV12:
		case VA_FOURCC_IYUV:
			imgfmt = IMGFMT_420P;
			break;
		case VA_FOURCC_NV12:
			imgfmt = IMGFMT_NV12;
			break;
		case VA_FOURCC('N', 'V', '2', '1'):
			imgfmt = IMGFMT_NV21;
			break;
		case VA_FOURCC_UYVY:
			imgfmt = IMGFMT_UYVY;
			break;
		case VA_FOURCC_YUY2:
			imgfmt = IMGFMT_YUYV;
			break;
		default:
			break;
		}
		mpi = mp_image_alloc(imgfmt, m_vaImage.width, m_vaImage.height);
		mpi->imgfmt = IMGFMT_VDPAU_FIRST;
//		Q_ASSERT(mpi->stride[0] >= m_vaImage.pitches[0]);
		for (int i=0; i<mpi->fmt.num_planes; ++i) {
			uchar *dest = mpi->planes[i];
			qDebug() << mpi->fmt.ys[i];
			const uchar *src = temp + m_vaImage.offsets[i];
			const int size = qMin(mpi->stride[i], (int)m_vaImage.pitches[i]);
			const int hmax = (m_vaImage.height >> mpi->fmt.ys[i]);
			for (int h=0; h<hmax; ++h) {
				memcpy(dest, src, size);
				dest += mpi->stride[i];
				src += m_vaImage.pitches[i];
			}
		}
//		mp_image_unrefp(&mpi);
	} while (0);
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "va(DeriveImage|MapBuffer)():" << vaErrorStr(status);
	cleanImage();
	return mpi;
}

static int control(sh_video_t *sh, int cmd, void *arg)
{
	VaApi *ctx = (VaApi*)sh->context;
	AVCodecContext *avctx = (AVCodecContext*)ctx->m_avctx;
	switch (cmd) {
//	case VDCTRL_QUERY_FORMAT:
//		return (*((int *)arg)) == IMGFMT_VDPAU ? CONTROL_TRUE : CONTROL_FALSE;
	case VDCTRL_RESYNC_STREAM:
		avcodec_flush_buffers(avctx);
		return CONTROL_TRUE;
	case VDCTRL_QUERY_UNSEEN_FRAMES: {
		int delay = avctx->has_b_frames;
		if (avctx->active_thread_type & FF_THREAD_FRAME)
			delay += avctx->thread_count - 1;
		return delay + 10;
	} case VDCTRL_RESET_ASPECT: {
		if (ctx->m_vo)
			ctx->m_vo = false;
		ctx->initVideoOutput(avctx->pix_fmt);
		return true;
	} default:
		return CONTROL_UNKNOWN;
	}
}

extern vd_functions cmplayer_vd_vaapi;
void VaApiInfo::initialize() {
	static const vd_info_t info = {
		"libva video codecs",
		"vaapi",
		"",
		"",
		"HwAcc codecs",
		"libva",
	};
	cmplayer_vd_vaapi.info = &info;
	cmplayer_vd_vaapi.init = VaApi::init;
	cmplayer_vd_vaapi.uninit = VaApi::uninit;
	cmplayer_vd_vaapi.control = control;
	cmplayer_vd_vaapi.decode = VaApi::decode;
	get();
}
