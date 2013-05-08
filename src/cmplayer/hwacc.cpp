#include "hwacc.hpp"
#include "stdafx.hpp"
extern "C" {
#include <video/filter/vf.h>
#include <video/decode/vd.h>
#include <video/img_format.h>
#include <demux/demux_packet.h>
#include <core/codecs.h>
#include <core/av_common.h>
#include <libavcodec/avcodec.h>
#ifdef Q_OS_LINUX
#include <video/mp_image_pool.h>
#include <va/va.h>
#include <va/va_glx.h>
#include <libavcodec/vaapi.h>
#endif
#ifdef Q_OS_MAC
#include <libavcodec/vda.h>
#endif
}

#ifdef Q_OS_LINUX
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow.h>
struct VaApiInfo {
	struct Codec { AVCodecID id = AV_CODEC_ID_NONE; QVector<VAProfile> profiles; int surfaceCount = 0; };
	static VaApiInfo &get();
	const Codec *find(AVCodecID id) const { auto it = m_supported.find(id); return it != m_supported.end() && !it->profiles.isEmpty() ? &(*it) : 0; }
	void *display() const { return m_display; }
	void finalize() {if (m_display) {vaTerminate(m_display); m_display = nullptr; init = false;}}
private:
	VaApiInfo() {
		m_xdpy = static_cast<Display*>(qApp->platformNativeInterface()->nativeResourceForWindow("display", qApp->topLevelWindows().first()));
		if (!m_xdpy || !(m_display = vaGetDisplayGLX(m_xdpy)))
			return;
		int major, minor;
		if (vaInitialize(m_display, &major, &minor) != VA_STATUS_SUCCESS)
			return;
		init = true;
		auto size = vaMaxNumProfiles(m_display);
		QVector<VAProfile> profiles;
		profiles.resize(size);
		if (vaQueryConfigProfiles(m_display, profiles.data(), &size) != VA_STATUS_SUCCESS)
			return;
		profiles.resize(size);
		auto supports = [this, &profiles](const QVector<VAProfile> &candidates, int count, AVCodecID id) {
			Codec codec;
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
		m_supported[AV_CODEC_ID_MPEG1VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG1VIDEO);
		m_supported[AV_CODEC_ID_MPEG2VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG2VIDEO);
		m_supported[AV_CODEC_ID_MPEG4] = supports(mpeg4s, NUM_VIDEO_SURFACES_MPEG4, AV_CODEC_ID_MPEG4);
		m_supported[AV_CODEC_ID_WMV3] = supports(wmv3s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_WMV3);
		m_supported[AV_CODEC_ID_VC1] = supports(vc1s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_VC1);
		m_supported[AV_CODEC_ID_H264] = supports(h264s, NUM_VIDEO_SURFACES_H264, AV_CODEC_ID_H264);
	}
	QMap<AVCodecID, Codec> m_supported;
	Display *m_xdpy = nullptr;
	VADisplay m_display = nullptr;
	static bool init;
	friend void finalize_vaapi();
};

bool VaApiInfo::init = false;

void finalize_vaapi() {if (VaApiInfo::init) VaApiInfo::get().finalize();}

VaApiInfo &VaApiInfo::get() {static VaApiInfo info; return info;}

struct VaApi {
	struct Texture {
		Texture(VADisplay display, int width, int height): display(display) {
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			status = vaCreateSurfaceGLX(display, GL_TEXTURE_2D, id, &surface);
			if (status != VA_STATUS_SUCCESS)
				qDebug() << "vaCreateSurfaceGLX():" << vaErrorStr(status);
		}
		~Texture() {
			if (surface)
				vaDestroySurfaceGLX(display, &surface);
			glDeleteTextures(1, &id);
		}
		VADisplay display = 0;
		GLuint id = GL_NONE;
		void *surface = nullptr;
		int status = VA_STATUS_ERROR_UNKNOWN;
	};
	struct VaApiSurface { VASurfaceID  id = VA_INVALID_ID; bool ref = false; quint64 order = 0; };
	VaApi() {
		memset(&m_context, 0, sizeof(m_context));
		m_context.context_id = m_context.config_id = VA_INVALID_ID;
		m_context.display = VaApiInfo::get().display();
		m_window.setSurfaceType(QWindow::OpenGLSurface);
		m_window.setGeometry(-10, -10, 1, 1);
		m_window.create();
		m_gl.create();
		m_gl.makeCurrent(&m_window);
		m_pool = mp_image_pool_new(5);
	}
	~VaApi() { freeContext(); mp_image_pool_clear(m_pool); talloc_free(m_pool); m_gl.doneCurrent(); m_window.destroy();}
	static constexpr const char *name() {return "vaapi";}
	static constexpr PixelFormat vld() {return AV_PIX_FMT_VAAPI_VLD;}
	static constexpr quint32 imgfmt() {return IMGFMT_BGRA;}
	static void addDecoders(mp_decoder_list *list) {
		mp_add_decoder(list, "vaapi", "mpegvideo", "mpegvideo", "VA-API MPEG-1/2");
		mp_add_decoder(list, "vaapi", "h264", "h264", "VA-API H.264");
		mp_add_decoder(list, "vaapi", "vc1", "vc1", "VA-API WVC1");
		mp_add_decoder(list, "vaapi", "mpeg4", "mpeg4", "VA-API MPEG-4,DIVX-4/5");
		mp_add_decoder(list, "vaapi", "wmv3", "wmv3", "VA-API WMV3/WMV9");
	}
	VASurfaceID data() {
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
		return m_surfaces[i].id;
	}
	void release(void *data) {
		const auto id = (VASurfaceID)(uintptr_t)data;
		for (int i=0; i<m_surfaces.size(); ++i) {
			if (m_surfaces[i].id == id) {
				m_surfaces[i].ref = false;
				break;
			}
		}
	}
	void freeContext() {
		if (m_context.display) {
			delete m_texture;
			if (m_context.context_id != VA_INVALID_ID)
				qDebug() << vaDestroyContext(m_context.display, m_context.context_id);
			for (auto &surface : m_surfaces) {
				if (surface.id != VA_INVALID_SURFACE)
					vaDestroySurfaces(m_context.display, &surface.id, 1);
			}
			if (m_context.config_id != VA_INVALID_ID)
				qDebug() << vaDestroyConfig(m_context.display, m_context.config_id);
		}
		m_context.context_id = m_context.config_id = VA_INVALID_ID;
		m_profile = (VAProfile)(-1);
		m_surfaces.clear();
		m_surfaceOrder = 0;
		m_texture = nullptr;
	}
	bool fillContext(AVCodecContext *avctx) {
		avctx->hwaccel_context = nullptr;
		auto codec = VaApiInfo::get().find(avctx->codec_id);
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
			if ((status = vaCreateSurfaces(m_context.display, avctx->width, avctx->height, VA_RT_FORMAT_YUV420, ids.size(), ids.data())) != VA_STATUS_SUCCESS
					&& (status = vaCreateSurfaces(m_context.display, avctx->width, avctx->height, VA_RT_FORMAT_YUV422, ids.size(), ids.data())) != VA_STATUS_SUCCESS)
				break;
			m_surfaces.resize(ids.size());
			for (int i=0; i<ids.size(); ++i)
				m_surfaces[i].id = ids[i];
			if ((status = vaCreateContext(m_context.display, m_context.config_id, avctx->width, avctx->height, VA_PROGRESSIVE, ids.data(), ids.size(), &m_context.context_id)) != VA_STATUS_SUCCESS)
				break;
			avctx->hwaccel_context = &m_context;
			auto img = mp_image_pool_get(m_pool, imgfmt(), avctx->width, avctx->height);
			m_texture = new Texture(m_context.display, img->stride[0]/4, img->h);
			talloc_free(img);
			if ((status = m_texture->status) != VA_STATUS_SUCCESS)
				break;
			return true;
		} while(0);
		if (status != VA_STATUS_SUCCESS)
			qDebug() << "vaapi:" << vaErrorStr(status);
		return false;
	}
	mp_image *image(AVFrame *pic) {
		const auto id = (VASurfaceID)(uintptr_t)pic->data[3];
		vaSyncSurface(m_context.display, id);
		glBindTexture(GL_TEXTURE_2D, m_texture->id);
		const auto status = vaCopySurfaceGLX(m_context.display, m_texture->surface, id, VA_SRC_BT601);
		if (status != VA_STATUS_SUCCESS)
			qDebug() << "vaCopySurfaceGLX():" << vaErrorStr(status);
		auto mpi = mp_image_pool_get(m_pool, imgfmt(), pic->width, pic->height);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, mpi->planes[0]);
		return mpi;
	}
private:
	vaapi_context m_context;
	QVector<VaApiSurface> m_surfaces;
	quint64 m_surfaceOrder = 0;
	VAProfile m_profile = (VAProfile)(-1);
	QWindow m_window;
	QOpenGLContext m_gl;
	Texture *m_texture = nullptr;
	mp_image_pool *m_pool = nullptr;

};
using Backend = VaApi;


bool HwAcc::supports(AVCodecID codec) { return VaApiInfo::get().find(codec) != nullptr; }
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

#ifdef Q_OS_MAC
struct Vda {
	Vda() { memset(&m_context, 0, sizeof(struct vda_context)); }
	~Vda() { freeContext(); }
	constexpr static const char *name() {return "vda";}
	quint32 imgfmt() const {return m_imgfmt;}
	constexpr static int data()  { return 1; }
	void release(void */*data*/) {	}
	static constexpr PixelFormat vld() {return AV_PIX_FMT_VDA_VLD;}
	static void addDecoders(mp_decoder_list *list) {
		mp_add_decoder(list, "vda", "h264", "h264", "Apple VDA H.264");
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
	void freeContext() {
		if (m_init) {
			ff_vda_destroy_decoder(&m_context);
			memset(&m_context, 0, sizeof(struct vda_context));
		}
	}
	bool fillContext(AVCodecContext *avctx) {
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
private:
	vda_context m_context;
	bool m_init = false;
	OSType m_bufferType = vdaType(AV_PIX_FMT_UYVY422);
	mp_imgfmt m_imgfmt = imgfmt(m_bufferType);
	mp_image m_mpi;
};
typedef Vda Backend;

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

struct HwAccDecoder {
	HwAccDecoder(sh_video *sh, const char *decoder);
	~HwAccDecoder();
	static int init(sh_video_t *sh, const char *decoder) {
		HwAccDecoder *dec = new HwAccDecoder(sh, decoder);
		if (dec->m_backend->fillContext(dec->m_avctx))
			return 1;
		delete dec;	return 0;
	}
	static void uninit(sh_video_t *sh) { delete static_cast<HwAccDecoder*>(sh->context); }
	static int get_buffer(AVCodecContext *avctx, AVFrame *pic) { return static_cast<HwAccDecoder*>(avctx->opaque)->getBuffer(pic); }
	static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic) { static_cast<HwAccDecoder*>(avctx->opaque)->releaseBuffer(pic); }
	static mp_image *decode(struct sh_video *sh, struct demux_packet *pkt, int flags, double *reordered_pts) {
		return static_cast<HwAccDecoder*>(sh->context)->decode(pkt, flags, reordered_pts);
	}
	mp_image *decode(struct demux_packet *pkt, int flags, double *reordered_pts);
	bool initVideoOutput(AVPixelFormat pix_fmt);
	int getBuffer(AVFrame *pic);
	void releaseBuffer(AVFrame *pic);
	static PixelFormat find(AVCodecContext *avctx, const PixelFormat *fmt) {
//		auto vaapi = static_cast<HwAccDecoder*>(avctx->opaque);
		for (int i = 0; fmt[i] != PIX_FMT_NONE; ++i) {
			if (fmt[i] == Backend::vld()/* && vaapi->initVideoOutput(fmt[i])*/)
				return fmt[i];
		}
		Q_ASSERT(false);
		return PIX_FMT_NONE;
	}
	AVCodecContext *m_avctx = nullptr;
	AVFrame *m_pic = nullptr;
	bool m_vo = false;
	AVRational m_last_sample_aspect_ratio;
	Backend *m_backend = nullptr;
	sh_video *m_sh = nullptr;
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
	m_backend = new Backend;
}

HwAccDecoder::~HwAccDecoder() {
	delete m_backend;
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
		if (!m_backend || !m_backend->fillContext(m_avctx) || !mpcodecs_config_vo(m_sh, m_sh->disp_w, m_sh->disp_h, m_backend->imgfmt()))
			return false;
		m_vo = true;
	}
	return true;
}

int HwAccDecoder::getBuffer(AVFrame *pic) {
	if (!initVideoOutput(m_avctx->pix_fmt))
		return -1;
	pic->linesize[0] = pic->linesize[1] = pic->linesize[2] = pic->linesize[3] = 0;
	pic->data[1] = pic->data[2] = nullptr;
	pic->data[0] = pic->data[3] = (uchar*)(quintptr)m_backend->data();
	pic->type = FF_BUFFER_TYPE_USER;
	pic->reordered_opaque = m_avctx->reordered_opaque;
	return 0;
}

void HwAccDecoder::releaseBuffer(AVFrame *pic) {
	m_backend->release(pic->data[3]);
	pic->data[0] = pic->data[1] = pic->data[2] = pic->data[3] = nullptr;
}

union pts { int64_t i; double d; };
mp_image *HwAccDecoder::decode(demux_packet *packet, int flags, double *reordered_pts) {
	if (flags & 2)
		m_avctx->skip_frame = AVDISCARD_ALL;
	else if (flags & 1)
		m_avctx->skip_frame = AVDISCARD_NONREF;
	else
		m_avctx->skip_frame = AVDISCARD_DEFAULT;

	AVPacket pkt; av_init_packet(&pkt);
	pkt.data = packet ? packet->buffer : nullptr;
	pkt.size = packet ? packet->len : 0;
	if (packet && packet->keyframe)
		pkt.flags |= AV_PKT_FLAG_KEY;
	if (packet && packet->avpacket) {
		pkt.side_data = packet->avpacket->side_data;
		pkt.side_data_elems = packet->avpacket->side_data_elems;
	}
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

QList<AVCodecID> HwAcc::fullCodecList() {
	static const QList<AVCodecID> list = QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO << AV_CODEC_ID_MPEG2VIDEO << AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3 << AV_CODEC_ID_VC1 << AV_CODEC_ID_H264;
	 return list;
}

extern vd_functions cmplayer_vd_hwacc;
HwAcc::HwAcc() {
	cmplayer_vd_hwacc.name = Backend::name();
	cmplayer_vd_hwacc.init = HwAccDecoder::init;
	cmplayer_vd_hwacc.uninit = HwAccDecoder::uninit;
	cmplayer_vd_hwacc.control = control;
	cmplayer_vd_hwacc.decode = HwAccDecoder::decode;
	cmplayer_vd_hwacc.add_decoders = Backend::addDecoders;
}
