#include "hwaccel.hpp"
#include "avmisc.hpp"

#define MAX_OUTPUT_SURFACES       2 /* Maintain synchronisation points in flip_page() */
#define MAX_VIDEO_SURFACES       21 /* Maintain free surfaces in a queue (use least-recently-used) */
#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */

extern int HWACCEL_FORMAT;
extern "C" void *fast_memcpy(void *to, const void *from, size_t len);

class VaProfileManager {
	QVector<VAProfile> m_profiles;
	VAProfile findMatchedProfile(const QVector<VAProfile> &needs) {
		for (auto need : needs) {
			if (m_profiles.contains(need))
				return need;
		}
		return NoProfile;
	}
	VaProfileManager(VADisplay display) {
		auto count = vaMaxNumProfiles(display);
		m_profiles.resize(count);
		if (vaQueryConfigProfiles(display, m_profiles.data(), &count))
			return;
		m_profiles.resize(count);
	}
public:
	static const VAProfile NoProfile = (VAProfile)(-1);
	static VaProfileManager &get(VADisplay display) {
		static VaProfileManager obj(display);
		return obj;
	}
	VAProfile find(CodecID codec, int &surfaceCount) {
		static const QVector<VAProfile> mpeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
		static const QVector<VAProfile> mpeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
		static const QVector<VAProfile> h264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};
		static const QVector<VAProfile> wmv3s = {VAProfileVC1Main, VAProfileVC1Simple};
		static const QVector<VAProfile> vc1s = {VAProfileVC1Advanced};
		switch(codec) {
		case CODEC_ID_MPEG1VIDEO:
		case CODEC_ID_MPEG2VIDEO:
			surfaceCount = NUM_VIDEO_SURFACES_MPEG2;
			return findMatchedProfile(mpeg2s);
		case CODEC_ID_MPEG4:
			surfaceCount = NUM_VIDEO_SURFACES_MPEG4;
			return findMatchedProfile(mpeg4s);
		case CODEC_ID_WMV3:
			surfaceCount = NUM_VIDEO_SURFACES_VC1;
			return findMatchedProfile(wmv3s);
		case CODEC_ID_VC1:
			surfaceCount = NUM_VIDEO_SURFACES_VC1;
			return findMatchedProfile(vc1s);
		case CODEC_ID_H264:
			surfaceCount = NUM_VIDEO_SURFACES_H264;
			return findMatchedProfile(h264s);
		default:
			break;
		}
		return NoProfile;
	}
};

HwAccel::HwAccel(AVCodecContext *avctx) {
#ifdef Q_WS_MAC
#endif
#ifdef Q_WS_X11
	if (!(m_x11 = XOpenDisplay(nullptr)))
		return;
	if (!(m_display = vaGetDisplayGLX(m_x11)))
		return;
	int major, minor;
	if (vaInitialize(m_display, &major, &minor))
		return;
	int count = 0;
	const auto profile = VaProfileManager::get(m_display).find(avctx->codec->id, count);
	if (profile == VaProfileManager::NoProfile)
		return;
	m_surfaces.resize(count);
	VAConfigAttrib attr;
	memset(&attr, 0, sizeof(attr));
	attr.type = VAConfigAttribRTFormat;
	if(vaGetConfigAttributes(m_display, profile, VAEntrypointVLD, &attr, 1))
		return;
	if(!(attr.value & VA_RT_FORMAT_YUV420))
		return;
	if(vaCreateConfig(m_display, profile, VAEntrypointVLD, &attr, 1, &m_configId)) {
		m_configId = VA_INVALID_ID;
		return;
	}
	initializeContext();

	makeCurrent();
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	doneCurrent();
#endif
	m_avctx = avctx;
	m_avctx->get_format = getFormat;
	m_avctx->get_buffer = getBuffer;
	m_avctx->release_buffer = releaseBuffer;
	m_avctx->reget_buffer = regetBuffer;
	m_avctx->draw_horiz_band = nullptr;
	m_usable = true;
}

HwAccel::~HwAccel() {
	if (m_avctx) {
		dtorContext();
		m_avctx->hwaccel_context = nullptr;
	}
#ifdef Q_WS_X11
	if (m_configId != VA_INVALID_ID)
		vaDestroyConfig(m_display, m_configId);
	if (m_display)
		vaTerminate(m_display);
	if (m_x11)
		XCloseDisplay(m_x11);
#endif
}

void HwAccel::initializeContext() {
	memset(&m_ctx, 0, sizeof(m_ctx));
	m_ctx.display = nullptr;
	m_ctx.config_id = m_ctx.context_id = VA_INVALID_ID;
}

bool HwAccel::ctorContext(AVCodecContext *avctx) {
	m_width = avctx->width;
	m_height = avctx->height;
	if (m_width <= 0 || m_height <= 0)
		return false;
#ifdef Q_WS_MAC
	ctx.decoder = nullptr;
	ctx.width = avctx->width;
	ctx.height = avctx->height;
	ctx.format = 'avc1';
	ctx.cv_pix_fmt_type = HWACCEL_FORMAT == IMGFMT_YUY2 ? kCVPixelFormatType_422YpCbCr8_yuvs : kCVPixelFormatType_420YpCbCr8Planar;
	ctx.use_sync_decoding = 1;
	imgFmt = HWACCEL_FORMAT;
	return ff_vda_create_decoder(&ctx, avctx->extradata, avctx->extradata_size) == 0;
#else
	m_ctx.display = m_display;
	m_ctx.config_id = m_configId;
	m_ctx.context_id = VA_INVALID_ID;

	QVector<VASurfaceID> ids(m_surfaces.size());
	if (vaCreateSurfaces(m_ctx.display, m_width, m_height, VA_RT_FORMAT_YUV420, ids.size(), ids.data())) {
		for (auto &surface : m_surfaces)
			surface.id = VA_INVALID_SURFACE;
		return false;
	}
	for (int i=0; i<m_surfaces.size(); ++i)
		m_surfaces[i].id = ids[i];
	if (vaCreateContext(m_ctx.display, m_ctx.config_id, m_width, m_height, VA_PROGRESSIVE, ids.data(), ids.size(), &m_ctx.context_id)) {
		m_ctx.context_id = VA_INVALID_ID;
		return false;
	}
	uint32_t fourcc = 0;
	int count = 0;
	if (m_derived) {
		for (int i=0; i<m_surfaces.size(); ++i, ++count) {
			if (vaDeriveImage(m_ctx.display, m_surfaces[i].id, &m_surfaces[i].image)) {
				m_derived = false;
				break;
			}
			if (fourcc && fourcc != m_surfaces[i].image.format.fourcc) {
				m_derived = false;
				++count;
				break;
			}
			fourcc = m_surfaces[i].image.format.fourcc;
		}
	}
	auto vaFourccToImgFmt = [] (uint32_t vaFourcc) {
		switch (vaFourcc) {
		case VA_FOURCC('Y', 'V', '1', '2'):
		case VA_FOURCC('I', '4', '2', '0'):
			return IMGFMT_YV12;
		case VA_FOURCC('N', 'V', '1', '2'):
			return IMGFMT_NV12;
		default:
			return 0;
		}
	};
	if (m_derived) {
		m_imgFmt = vaFourccToImgFmt(fourcc);
		if (!m_imgFmt)
			m_derived = false;
	}
	if (!m_derived) {
		qDebug() << "failed on vaDeriveImage(); fall back to vaCreateImage();";
		for (int i=0; i<count; ++i)
			vaDestroyImage(m_ctx.display, m_surfaces[i].image.image_id);
		int formatCount = vaMaxNumImageFormats(m_ctx.display);
		QVector<VAImageFormat> formats(formatCount);
		if (vaQueryImageFormats(m_ctx.display, formats.data(), &formatCount))
			return false;
		m_imgFmt = 0;
		VAImageFormat *format = nullptr;
		VAImage &vaImage = m_surfaces[0].image;
		for (int i=0; i<formatCount && !m_imgFmt; ++i) {
			auto &fmt = formats[i];
			if (vaCreateImage(m_ctx.display, &fmt, m_width, m_height, &vaImage)) {
				vaImage.image_id = VA_INVALID_ID;
				continue;
			}
			if (vaGetImage(m_ctx.display, ids[0], 0, 0, m_width, m_height, vaImage.image_id)) {
				vaDestroyImage(m_ctx.display, vaImage.image_id);
				vaImage.image_id = VA_INVALID_ID;
				continue;
			}
			format = &fmt;
			break;
		}
		if (!format)
			return false;
		m_imgFmt = vaFourccToImgFmt(format->fourcc);
		for (int i=1; i<m_surfaces.size(); ++i)
			vaCreateImage(m_ctx.display, format, m_width, m_height, &m_surfaces[i].image);
	}
	makeCurrent();
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	int status = vaCreateSurfaceGLX(m_ctx.display, GL_TEXTURE_2D, m_texture,	&m_glSurface);
	doneCurrent();
	qDebug() << "vaCreatteSurfaceGLX():" << vaErrorStr(status);
	return true;
#endif
}

void HwAccel::dtorContext() {
#ifdef Q_WS_MAC
		if (ctx.decoder)
			ff_vda_destroy_decoder(&ctx);
		memset(&ctx, 0, sizeof(ctx));
#else
	if (m_ctx.display) {
		if (m_glSurface) {
			makeCurrent();
			glBindTexture(GL_TEXTURE_2D, m_texture);
			vaDestroySurfaceGLX(m_ctx.display, &m_glSurface);
			doneCurrent();
		}
		for (auto &s : m_surfaces) {
			if (s.image.image_id != VA_INVALID_ID)
				vaDestroyImage(m_ctx.display, s.image.image_id);
		}
		if (m_ctx.context_id != VA_INVALID_ID)
			vaDestroyContext(m_ctx.display, m_ctx.context_id);
		for (auto &surface : m_surfaces) {
			if (surface.id != VA_INVALID_SURFACE)
				vaDestroySurfaces(m_ctx.display, &surface.id, 1);
		}
		m_surfaces.clear();
	}
	m_glSurface = nullptr;
#endif
	initializeContext();
	m_width = m_height = 0;
}

bool HwAccel::setup() {
	if (initCtx && m_avctx->width == m_width && m_avctx->height == m_height)
		return true;
	dtorContext();
	if ((initCtx = ctorContext(m_avctx))) {
		m_avctx->hwaccel_context = &m_ctx;
		return true;
	} else {
		m_avctx->hwaccel_context = nullptr;
		dtorContext();
		return false;
	}
}

PixelFormat HwAccel::getFormat(AVCodecContext *avctx, const PixelFormat *pixFmt) {
	auto hwaccel = get(avctx);
	if (!hwaccel)
		return PIX_FMT_NONE;
	auto sh = _sh(avctx);
#ifdef Q_WS_MAC
	const auto format = PIX_FMT_VDA_VLD;
#else
	const auto format = PIX_FMT_VAAPI_VLD;
#endif
	int i = 0;
	for (; pixFmt[i] != PIX_FMT_NONE; ++i) {
		if (format == pixFmt[i] && init_vo(sh, pixFmt[i]) >= 0 && hwaccel->setup()) {
			qDebug() << "Found HwAccel!";
			break;
		}
	}
	return pixFmt[i];
}

void HwAccel::releaseBuffer(AVFrame *frame) {
#ifdef Q_WS_MAC
	auto cv_buffer = reinterpret_cast<CVPixelBufferRef>(frame->data[3]);
	if (cv_buffer)
		CFRelease(cv_buffer);
#else
	auto surface = (VaSurface*)(uintptr_t)frame->data[0];
	auto id = (VASurfaceID)(uintptr_t)frame->data[3];
	Q_ASSERT(surface->id == id);
	Q_ASSERT(surface->bound);
	surface->bound = false;
#endif
	frame->data[0] = frame->data[1] = frame->data[2] = frame->data[3] = nullptr;
}

bool HwAccel::fill(mp_image_t *image, AVFrame *frame) {
#ifdef Q_WS_MAC
	CVPixelBufferRef buffer = (CVPixelBufferRef)(frame->data[3]);
	CVPixelBufferLockBaseAddress(buffer, 0);
	image->planes[0] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 0);
	image->planes[1] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 1);
	image->planes[2] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 2);
	image->stride[0] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
	image->stride[1] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
	image->stride[2] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 2);
	CVPixelBufferUnlockBaseAddress(buffer, 0);
#else
	VaSurface *surface = (VaSurface*)(uintptr_t)frame->data[0];
	VASurfaceID id = (VASurfaceID)(uintptr_t)frame->data[3];
	auto &vaImage = surface->image;
	if (!m_derived && vaGetImage(m_ctx.display, id, 0, 0, m_width, m_height, vaImage.image_id))
		return false;
//	makeCurrent();
//	qDebug() << vaErrorStr(vaCopySurfaceGLX(m_ctx.display, m_glSurface, surface->id, VA_FRAME_PICTURE | VA_SRC_BT601));
//	doneCurrent();
	void *buffer = nullptr;
	if (vaMapBuffer(m_ctx.display, vaImage.buf, &buffer))
		return false;
	int height[3] = {vaImage.height, vaImage.height >> 1, vaImage.height >> 1};
	auto setPlane = [this, buffer, image, &vaImage, &height] (int destIdx, int srcIdx) {
		const int len = height[srcIdx]*vaImage.pitches[srcIdx];
		m_buffer[destIdx].resize(len);
		uchar *dest = (uchar*)m_buffer[destIdx].data();
		uchar *src = (uchar*)buffer + vaImage.offsets[srcIdx];
		fast_memcpy(dest, src, len);
		image->planes[destIdx] = dest;
		image->stride[destIdx] = vaImage.pitches[srcIdx];
	};
	for (uint i=0; i<vaImage.num_planes; ++i)
		setPlane(i, i);
	const auto fourcc = vaImage.format.fourcc;
	if (fourcc == VA_FOURCC('I', '4', '2', '0'))
		qSwap(image->planes[1], image->planes[2]);
	if(vaUnmapBuffer(m_ctx.display, vaImage.buf))
		return false;
#endif
	return true;
}

bool HwAccel::getBuffer(AVFrame *frame) {
	frame->reordered_opaque = m_avctx->reordered_opaque;
	frame->opaque = nullptr;
	frame->type = FF_BUFFER_TYPE_USER;
	for (int i=0; i<4; ++i) {
		frame->data[i] = nullptr;
		frame->linesize[i] = 0;
	}
	frame->type = FF_BUFFER_TYPE_USER;
	setup();
#ifdef Q_WS_MAC
	frame->data[0] = frame->data[3] = reinterpret_cast<uint8_t*>(1);
#else
	VaSurface *surface = nullptr;
	for (auto &s : m_surfaces) {
		if (!s.bound) {
			surface = &s;
			break;
		}
	}
	if (!surface) {
		qDebug() << "no free buffer! something's going wrong!";
		surface = &m_surfaces.first();
	}
	surface->bound = true;
	frame->data[0] = (uchar*)(uintptr_t)surface;
	frame->data[3] = (uchar*)(uintptr_t)(surface->id);
#endif
	return true;
}


int cmplayer_hwaccel_fill_image(void *hwaccel, mp_image_t *image, AVFrame *frame) {
	return reinterpret_cast<HwAccel*>(hwaccel)->fill(image, frame);
}

uint32_t cmplayer_hwaccel_setup(void *hwaccel) {
	auto hwa = reinterpret_cast<HwAccel*>(hwaccel);
	return hwa->setup() ? hwa->m_imgFmt : 0;
}

int HwAccel::getBuffer(AVCodecContext *avctx, AVFrame *frame) {
	return get(avctx)->getBuffer(frame) ? 0 : -1;
}

int HwAccel::regetBuffer(AVCodecContext *avctx, AVFrame *frame) {
	frame->reordered_opaque = avctx->reordered_opaque;
	return avcodec_default_reget_buffer(avctx, frame);
}

void HwAccel::releaseBuffer(AVCodecContext *avctx, AVFrame *frame) {
	get(avctx)->releaseBuffer(frame);
}

void *cmplayer_hwaccel_create(AVCodecContext *avctx) {
	if (!HWACCEL_FORMAT)
		return nullptr;
	auto hwaccel = new HwAccel(avctx);
	if (hwaccel->isUsable())
		return hwaccel;
	delete hwaccel;
	return nullptr;
}

void cmplayer_hwaccel_delete(void **hwaccel) {
	if (*hwaccel) {
		delete reinterpret_cast<HwAccel*>(*hwaccel);
		*hwaccel = nullptr;
	}
}
