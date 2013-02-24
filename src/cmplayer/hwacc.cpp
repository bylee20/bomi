#include "hwacc.hpp"

QList<AVCodecID> HwAcc::fullCodecList() {
	static const QList<AVCodecID> list = QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO << AV_CODEC_ID_MPEG2VIDEO << AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3 << AV_CODEC_ID_VC1 << AV_CODEC_ID_H264;
	 return list;
}

#ifdef Q_OS_MAC
HwAcc::~HwAcc() {}
bool HwAcc::supports(AVCodecID codec) { return codec == AV_CODEC_ID_H264; }
const char *HwAcc::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "vda:h264";
	default:
		return nullptr;
	}
}
#endif

#ifdef Q_OS_LINUX
#include "mpv-vaapi.hpp"
HwAcc::~HwAcc() { VaApiInfo::finalize(); }
bool HwAcc::supports(AVCodecID codec) { return VaApiInfo::find(codec) != nullptr; }
const char *HwAcc::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "vaapi:h264";
	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		return "vaapi:mpegvideo";
	case AV_CODEC_ID_MPEG4:
		return "vaapi:mpeg4";
	case AV_CODEC_ID_WMV3:
		return "vaapi:wmv3";
	case AV_CODEC_ID_VC1:
		return "vaapi:vc1";
	default:
		return nullptr;
	}
}
#endif

extern "C" {
#include "video/fmt-conversion.h"
#include <video/filter/vf.h>
#include <video/decode/vd.h>
#include "video/img_format.h"
#include "demux/demux_packet.h"
#include "core/codecs.h"
#include "video/mp_image_pool.h"
#include "core/av_common.h"
#ifdef Q_OS_LINUX
#include <va/va.h>
#include <va/va_glx.h>
#include <libavcodec/vaapi.h>
#endif
#ifdef Q_OS_MAC
#include <libavcodec/vda.h>
#endif
#include <libavcodec/avcodec.h>
}
#include "stdafx.hpp"

#ifdef Q_OS_LINUX
struct VaApiCodec { AVCodecID id = AV_CODEC_ID_NONE; QVector<VAProfile> profiles; int surfaceCount = 0; };
struct VaApiInfo::Data { QMap<AVCodecID, VaApiCodec> supported; Display *xdpy = nullptr; VADisplay display = nullptr; };

VaApiInfo::VaApiInfo(): d(new Data) {
	if (!(d->xdpy = XOpenDisplay(NULL)) || !(d->display = vaGetDisplayGLX(d->xdpy)))
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
#endif

struct Vda {
	Vda(AVCodecContext *avctx) {
		memset(&m_context, 0, sizeof(struct vda_context));
		m_ok = true;
	}

	constexpr static const char *name() {return "vda";}

	static void addDecoders(mp_decoder_list *list) {
		mp_add_decoder(list, "vda", "h264", "h264", "Apple VDA H.264");
		return;

		mp_add_decoder(list, "vaapi", "mpegvideo", "mpegvideo", "VA-API MPEG-1/2");
		mp_add_decoder(list, "vaapi", "h264", "h264", "VA-API H.264");
		mp_add_decoder(list, "vaapi", "vc1", "vc1", "VA-API WVC1");
		mp_add_decoder(list, "vaapi", "mpeg4", "mpeg4", "VA-API MPEG-4,DIVX-4/5");
		mp_add_decoder(list, "vaapi", "wmv3", "wmv3", "VA-API WMV3/WMV9");
	}

	static OSType vdaType(AVPixelFormat pixfmt) {
		 switch (pixfmt) {
		 case AV_PIX_FMT_UYVY422:
			 return kCVPixelFormatType_422YpCbCr8;
		 case AV_PIX_FMT_YUYV422:
			 return kCVPixelFormatType_422YpCbCr8_yuvs;
		 case AV_PIX_FMT_NV12:
			 return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
		 case AV_PIX_FMT_YUV420P:
			 return kCVPixelFormatType_420YpCbCr8Planar;
		 default:
			 return 0;
		 }
	}

	static mp_imgfmt imgfmt(OSType type) {
		 switch (type) {
		 case kCVPixelFormatType_422YpCbCr8:
			 return IMGFMT_UYVY;
		 case kCVPixelFormatType_422YpCbCr8_yuvs:
			 return IMGFMT_YUYV;
		 case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
			 return IMGFMT_NV12;
		 case kCVPixelFormatType_420YpCbCr8Planar:
			 return IMGFMT_420P;
		 default:
			 return IMGFMT_NONE;
		 }
	}


	~Vda() { freeContext(); }


	constexpr static int data()  { return 1; }

	void release(void */*data*/) {	}

	static constexpr PixelFormat vld() {return AV_PIX_FMT_VDA_VLD;}
	AVPixelFormat pixfmt() const {return m_pixfmt;}
	quint32 imgfmt() const {return m_imgfmt;}
	void freeContext() {
		if (m_ok && m_init) {
			ff_vda_destroy_decoder(&m_context);
			memset(&m_context, 0, sizeof(struct vda_context));
		}
	}
	bool fillContext(AVCodecContext *avctx) {
		if (!m_ok)
			return false;
		freeContext();
		avctx->hwaccel_context = nullptr;
		m_context.width = avctx->width;
		m_context.height = avctx->height;
		m_context.format = 'avc1';

		m_context.cv_pix_fmt_type = m_bufferType;
		m_context.use_sync_decoding = 1;
		if (kVDADecoderNoErr != ff_vda_create_decoder(&m_context, avctx->extradata, avctx->extradata_size))
			return false;
		avctx->hwaccel_context = &m_context;
		memset(&m_mpi, 0, sizeof(m_mpi));
		mp_image_set_size(&m_mpi, avctx->width, avctx->height);
		mp_image_setfmt(&m_mpi, imgfmt());

		m_init = true;
		return true;
	}

	static void freeBuffer(void *arg) {
		CVPixelBufferRef buffer = (CVPixelBufferRef)arg;
		CVPixelBufferUnlockBaseAddress(buffer, 0);
		CVPixelBufferRelease(buffer);
	}

	mp_image *image(AVFrame *pic) {
		CVPixelBufferRef buffer = (CVPixelBufferRef)pic->data[3];
		CVPixelBufferLockBaseAddress(buffer, 0);
		if (m_mpi.w != pic->width || m_mpi.h != pic->height)
			mp_image_set_size(&m_mpi, pic->width, pic->height);
		if (CVPixelBufferIsPlanar(buffer)) {
			for (int i=0; i<m_mpi.num_planes; ++i) {
				m_mpi.planes[i] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, i);
				m_mpi.stride[i] = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
			}
		} else {
			m_mpi.planes[0] = (uchar*)CVPixelBufferGetBaseAddress(buffer);
			m_mpi.stride[0] = CVPixelBufferGetBytesPerRow(buffer);
		}
		return mp_image_new_custom_ref(&m_mpi, buffer, freeBuffer);
	}

	bool isOk() const {return m_ok;}
	bool m_ok = false;
	vda_context m_context;
	bool m_init = false;
	AVPixelFormat m_pixfmt = AV_PIX_FMT_UYVY422;
	OSType m_bufferType = vdaType(m_pixfmt);
	mp_imgfmt m_imgfmt = imgfmt(m_bufferType);
	mp_image m_mpi;
//	mp_image_pool *m_pool = nullptr;
};

typedef Vda Backend;

struct HwAccDecoder {
	HwAccDecoder(sh_video *sh, const char *decoder);
	~HwAccDecoder();
	static int init(sh_video_t *sh, const char *decoder) { HwAccDecoder *dec = new HwAccDecoder(sh, decoder); return dec->m_backend->isOk() ? 1 : ((delete decoder), 0); }
	static void uninit(sh_video_t *sh) { delete static_cast<HwAccDecoder*>(sh->context); }
	static int get_buffer(AVCodecContext *avctx, AVFrame *pic) { return static_cast<HwAccDecoder*>(avctx->opaque)->getBuffer(pic); }
	static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic) { static_cast<HwAccDecoder*>(avctx->opaque)->releaseBuffer(pic); }
	static mp_image *decode(sh_video *sh, demux_packet *packet, void *data, int len, int flags, double *reordered_pts) {
		return static_cast<HwAccDecoder*>(sh->context)->decode(packet, data, len, flags, reordered_pts);
	}
	mp_image *decode(demux_packet *packet, void *data, int len, int flags, double *reordered_pts);
	bool initVideoOutput(AVPixelFormat pix_fmt);
	int getBuffer(AVFrame *pic);
	void releaseBuffer(AVFrame *pic);
	static PixelFormat find(AVCodecContext *avctx, const PixelFormat *fmt) {
		auto vaapi = static_cast<HwAccDecoder*>(avctx->opaque);
		for (int i = 0; fmt[i] != PIX_FMT_NONE; i++) {
			if (fmt[i] == Backend::vld() && vaapi->initVideoOutput(fmt[i]))
				return fmt[i];
		}
		return PIX_FMT_NONE;
	}

	AVCodecContext *m_avctx = nullptr;
	AVFrame *m_pic = nullptr;
	bool m_vo = false, m_ok = false;
	AVRational m_last_sample_aspect_ratio;
	Backend *m_backend = nullptr;
	sh_video *m_sh = nullptr;
#ifdef Q_OS_LINUX
	QVector<VaApiSurface> m_surfaces;
	quint64 m_surfaceOrder = 0;
	vaapi_context m_context;
	VAProfile m_profile = (VAProfile)(-1);
	QWindow m_window;
	QOpenGLContext m_gl;
	Texture *m_texture = nullptr;
#endif
	mp_image_pool *m_pool = nullptr;
};

HwAccDecoder::HwAccDecoder(sh_video *sh, const char *decoder) {
	(m_sh = sh)->context = this;
	AVCodec *codec = avcodec_find_decoder_by_name(decoder);
	if (!codec)
		return;
	m_pic = avcodec_alloc_frame();
	m_avctx = avcodec_alloc_context3(codec);
	m_avctx->opaque = this;
	m_avctx->codec_type = AVMEDIA_TYPE_VIDEO;
	m_avctx->codec_id = codec->id;
	m_avctx->get_format = find;
	m_avctx->get_buffer = get_buffer;
	m_avctx->release_buffer = release_buffer;
	m_avctx->reget_buffer = get_buffer;
	m_avctx->draw_horiz_band = nullptr;
	m_avctx->slice_flags = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
	m_avctx->flags |= CODEC_FLAG_EMU_EDGE;
	m_avctx->coded_width = sh->disp_w;
	m_avctx->coded_height = sh->disp_h;
	m_avctx->codec_tag = sh->format;
	m_avctx->stream_codec_tag = sh->video.fccHandler;

	if (sh->gsh->lav_headers)
		   mp_copy_lav_codec_headers(m_avctx, sh->gsh->lav_headers);
	if (sh->bih && sh->bih->biSize > (int)sizeof(*sh->bih)) {
		m_avctx->extradata_size = sh->bih->biSize - sizeof(*sh->bih);
		m_avctx->extradata = (uchar*)av_mallocz(m_avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
		memcpy(m_avctx->extradata, sh->bih + 1, m_avctx->extradata_size);
	}
	if (sh->bih) {
		m_avctx->bits_per_coded_sample = sh->bih->biBitCount;
		m_avctx->coded_width  = sh->bih->biWidth;
		m_avctx->coded_height = sh->bih->biHeight;
	}
	m_avctx->thread_count = 1;
	if (avcodec_open2(m_avctx, codec, nullptr) < 0)
		return;
	m_backend = new Backend(m_avctx);
#ifdef Q_OS_LINUX
	m_window.setSurfaceType(QWindow::OpenGLSurface);
	m_window.setGeometry(-10, -10, 1, 1);
//	m_window.moveToThread(QThread::currentThread());
	m_window.create();
//	m_gl.moveToThread(QThread::currentThread());
	m_gl.create();
	m_gl.makeCurrent(&m_window);
#endif
	m_pool = mp_image_pool_new(5);
	m_ok = true;
}

HwAccDecoder::~HwAccDecoder() {
	delete m_backend;
//	freeContext();
	if (m_avctx) {
		if (m_avctx->codec && avcodec_close(m_avctx) < 0)
			mp_tmsg(MSGT_DECVIDEO, MSGL_ERR, "Could not close codec.\n");
		av_freep(&m_avctx->extradata);
		av_freep(&m_avctx->slice_offset);
	}
	av_freep(&m_avctx);
	avcodec_free_frame(&m_pic);
	mp_image_pool_clear(m_pool);
	m_sh->context = nullptr;
}

bool HwAccDecoder::initVideoOutput(PixelFormat pixfmt) {
	if (pixfmt != Backend::vld())
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
		if (!m_backend->fillContext(m_avctx) || !mpcodecs_config_vo(m_sh, m_sh->disp_w, m_sh->disp_h, m_backend->imgfmt()))
			return false;
		m_vo = true;
	}
	return true;
}

int HwAccDecoder::getBuffer(AVFrame *pic) {
	if (!initVideoOutput(m_avctx->pix_fmt))
		return -1;
//	m_backend->setBuffer(pic);
	//	int i_old, i;
	//	for (i=0, i_old=0; i<m_surfaces.size(); ++i) {
	//		if (!m_surfaces[i].ref)
	//			break;
	//		if (m_surfaces[i].order < m_surfaces[i_old].order)
	//			i_old = i;
	//	}
	//	if (i >= m_surfaces.size())
	//		i = i_old;
	//	m_surfaces[i].ref = true;
	//	m_surfaces[i].order = ++m_surfaceOrder;
	//	Q_ASSERT(m_surfaces[i].id != VA_INVALID_ID);
	//	uchar *dummy = (uchar*)(uintptr_t)m_surfaces[i].id;
//		uchar *dummy = reinterpret_cast<uint8_t*>(1);
		pic->linesize[0] = pic->linesize[1] = pic->linesize[2] = pic->linesize[3] = 0;
	//	pic->data[1] = pic->data[2] = nullptr;
		pic->data[0] = pic->data[3] = reinterpret_cast<uchar*>(m_backend->data());
		pic->type = FF_BUFFER_TYPE_USER;
		pic->reordered_opaque = m_avctx->reordered_opaque;
	return 0;
}

void HwAccDecoder::releaseBuffer(AVFrame *pic) {
	m_backend->release(pic->data[3]);

//	const auto id = (VASurfaceID)(uintptr_t)pic->data[3];
//	for (int i=0; i<m_surfaces.size(); ++i) {
//		if (m_surfaces[i].id == id) {
//			m_surfaces[i].ref = false;
//			break;
//		}
//	}
	pic->data[0] = pic->data[1] = pic->data[2] = pic->data[3] = nullptr;
}



union pts { int64_t i; double d; };
mp_image *HwAccDecoder::decode(demux_packet *packet, void *data, int len, int flags,  double *reordered_pts) {
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
	return m_backend->image(m_pic);
//	auto mpi = mp_image_pool_new_copy(m_pool, &tmp);

//	const auto id = (VASurfaceID)(uintptr_t)m_pic->data[3];
//	vaSyncSurface(m_context.display, id);
//	glBindTexture(GL_TEXTURE_2D, m_texture->id);
//	const auto status = vaCopySurfaceGLX(m_context.display, m_texture->surface, id, 0);
//	if (status != VA_STATUS_SUCCESS)
//		qDebug() << "vaCopySurfaceGLX():" << vaErrorStr(status);
//	auto mpi = mp_image_pool_get(m_pool, IMGFMT_BGRA, m_avctx->width, m_avctx->height);
//	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, mpi->planes[0]);
//	return mpi;
}

static int control(sh_video_t *sh, int cmd, void */*arg*/) {
	HwAccDecoder *ctx = (HwAccDecoder*)sh->context;
	AVCodecContext *avctx = (AVCodecContext*)ctx->m_avctx;
	switch (cmd) {
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

const char *HwAcc::name() { return Backend::name(); }

extern vd_functions cmplayer_vd_hwacc;
HwAcc::HwAcc() {
	cmplayer_vd_hwacc.name = Backend::name();
	cmplayer_vd_hwacc.init = HwAccDecoder::init;
	cmplayer_vd_hwacc.uninit = HwAccDecoder::uninit;
	cmplayer_vd_hwacc.control = control;
	cmplayer_vd_hwacc.decode = HwAccDecoder::decode;
	cmplayer_vd_hwacc.add_decoders = Backend::addDecoders;
#ifdef Q_OS_LINUX
	VaApiInfo::get();
#endif
}
