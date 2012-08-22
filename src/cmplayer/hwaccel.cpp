#include "videoframe.hpp"
#include "pref.hpp"
#include "hwaccel.hpp"
#ifdef Q_WS_X11
#include <QtGui/QX11Info>
#endif

extern "C" int is_hwaccel_available(AVCodecContext *avctx) {
	if (!Pref::get().enable_hwaccel || !Pref::get().hwaccel_codecs.contains(avctx->codec->id))
		return false;
#ifdef Q_WS_X11
	int count;
	if (HwAccelInfo::get().find(avctx, count) == HwAccelInfo::NoProfile)
		return false;
#endif
	qDebug() << "HwAccel is available!";
	return true;
}

HwAccelInfo *HwAccelInfo::obj = nullptr;

HwAccelInfo::HwAccelInfo() {
#ifdef Q_WS_X11
	if (!(m_display = vaGetDisplayGLX(QX11Info::display())))
		return;
	int major, minor;
	if (vaInitialize(m_display, &major, &minor))
		return;
	auto count = vaMaxNumProfiles(m_display);
	m_profiles.resize(count);
	if (vaQueryConfigProfiles(m_display, m_profiles.data(), &count))
		return;
	m_profiles.resize(count);
#endif
	m_ok = true;
}

HwAccelInfo::~HwAccelInfo() {
#ifdef Q_WS_X11
	if (m_display)
		vaTerminate(m_display);
#endif
}

QList<CodecID> HwAccelInfo::fullCodecList() const {
	return QList<CodecID>()
		<< CODEC_ID_MPEG1VIDEO
		<< CODEC_ID_MPEG2VIDEO
		<< CODEC_ID_MPEG4
		<< CODEC_ID_WMV3
		<< CODEC_ID_VC1
		<< CODEC_ID_H264;
}

bool HwAccelInfo::supports(CodecID codec) const {
#ifdef Q_WS_X11
	int count = 0;
	return find(codec, count) != NoProfile;
#endif
}

VAProfile HwAccelInfo::find(AVCodecContext *avctx, int &surfaceCount) const {
	const auto profile = find(avctx->codec->id, surfaceCount);
	m_avctx = profile != NoProfile ? avctx : nullptr;
	return profile;
}

VAProfile HwAccelInfo::find(CodecID codec, int &surfaceCount) const {
	static const QVector<VAProfile> mpeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
	static const QVector<VAProfile> mpeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
	static const QVector<VAProfile> h264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};
	static const QVector<VAProfile> wmv3s = {VAProfileVC1Main, VAProfileVC1Simple};
	static const QVector<VAProfile> vc1s = {VAProfileVC1Advanced};
#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */
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
	m_avctx = nullptr;
	return NoProfile;
}

VAProfile HwAccelInfo::findMatchedProfile(const QVector<VAProfile> &needs) const {
	if (!m_ok)
		return NoProfile;
	for (auto need : needs) {
		if (m_profiles.contains(need))
			return need;
	}
	return NoProfile;
}

HwAccel::HwAccel(AVCodecContext *avctx) {
	const auto &info = HwAccelInfo::get();
	if (!info.isAvailable())
		return;
	memset(&m_ctx, 0, sizeof(m_ctx));
	m_width = avctx->width;
	m_height = avctx->height;
	if (m_width <= 0 || m_height <= 0)
		return;
#ifdef Q_WS_MAC
	m_ctx.width = avctx->width;
	m_ctx.height = avctx->height;
	m_ctx.format = 'avc1';
	m_ctx.cv_pix_fmt_type = kCVPixelFormatType_420YpCbCr8Planar;
	m_ctx.use_sync_decoding = 1;
	if (ff_vda_create_decoder(&m_ctx, avctx->extradata, avctx->extradata_size))
		return;
#endif
#ifdef Q_WS_X11
	int count = 0;
	m_profile = info.find(avctx->codec->id, count);
	if (m_profile == HwAccelInfo::NoProfile)
		return;
	m_ids = QVector<VASurfaceID>(count, VA_INVALID_SURFACE);
	VAConfigAttrib attr;
	memset(&attr, 0, sizeof(attr));
	attr.type = VAConfigAttribRTFormat;
	if(vaGetConfigAttributes(info.display(), m_profile, VAEntrypointVLD, &attr, 1))
		return;
	if(!(attr.value & VA_RT_FORMAT_YUV420))
		return;

	m_ctx.display = info.display();
	m_ctx.config_id = VA_INVALID_ID;
	m_ctx.context_id = VA_INVALID_ID;
	if(vaCreateConfig(info.display(), m_profile, VAEntrypointVLD, &attr, 1, &m_ctx.config_id))
		return;
	if (vaCreateSurfaces(m_ctx.display, m_width, m_height, VA_RT_FORMAT_YUV420, m_ids.size(), m_ids.data()))
		return;
	if (vaCreateContext(m_ctx.display, m_ctx.config_id, m_width, m_height, VA_PROGRESSIVE, m_ids.data(), m_ids.size(), &m_ctx.context_id))
		return;
	for (auto id : m_ids)
		m_freeIds.push_back(id);
#endif
	m_avctx = avctx;
	m_avctx->hwaccel_context = &m_ctx;
	m_usable = true;
}

HwAccel::~HwAccel() {
	const auto &info = HwAccelInfo::get();
	if (!info.isAvailable())
		return;
#ifdef Q_WS_MAC
	if (m_ctx.decoder)
		ff_vda_destroy_decoder(&m_ctx);
#endif
#ifdef Q_WS_X11
	if (m_ctx.display) {
		if (m_glSurface)
			vaDestroySurfaceGLX(m_ctx.display, &m_glSurface);
		if (m_ctx.context_id != VA_INVALID_ID)
			vaDestroyContext(m_ctx.display, m_ctx.context_id);
		for (auto id : m_ids) {
			if (id != VA_INVALID_SURFACE)
				vaDestroySurfaces(m_ctx.display, &id, 1);
		}
		if (m_ctx.config_id != VA_INVALID_ID)
			vaDestroyConfig(m_ctx.display, m_ctx.config_id);
	}
#endif
}

VideoFormat HwAccel::format() const {
	auto format = VideoFormat::fromType(VideoFormat::BGRA, m_width, m_height);
	format.width_stride = format.stride = m_width;
	return format;
}

bool HwAccel::isCompatibleWith(const AVCodecContext *avctx) const {
	if (!m_usable || m_width != avctx->width || m_height != avctx->height)
		return false;
	int count = 0;
	auto profile = HwAccelInfo::get().find(avctx->codec->id, count);
	return profile != HwAccelInfo::NoProfile && profile == m_profile && m_ids.size() == count;
}

bool HwAccel::createSurface(GLuint *textures) {
	m_textures = textures;
#ifdef Q_WS_X11
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	int status = vaCreateSurfaceGLX(m_ctx.display, GL_TEXTURE_2D, m_textures[0], &m_glSurface);
	qDebug() << "vaCreatteSurfaceGLX():" << vaErrorStr(status);
	return status == VA_STATUS_SUCCESS;
#endif
}

bool HwAccel::copySurface(mp_image_t *mpi) {
#ifdef Q_WS_MAC
	CVPixelBufferRef buffer = (CVPixelBufferRef)(mpi->data[3]);
	CVPixelBufferLockBaseAddress(buffer, 0);
	auto setTex = [this] (int idx, int width, int height, const uchar *data) {
		glBindTexture(GL_TEXTURE_2D, m_texture(idx));
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	};
	setTex(0, m_width, m_height, CVPixelBufferGetBaseAddressOfPlane(buffer, 0));
	setTex(1, m_width >> 1, m_height >> 1, CVPixelBufferGetBaseAddressOfPlane(buffer, 1));
	setTex(2, m_width >> 1, m_height >> 1, CVPixelBufferGetBaseAddressOfPlane(buffer, 2));
//	image->stride[0] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
//	image->stride[1] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
//	image->stride[2] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 2);
	CVPixelBufferUnlockBaseAddress(buffer, 0);
#endif
#ifdef Q_WS_X11
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	VASurfaceID id = (VASurfaceID)(uintptr_t)mpi->planes[3];
	const auto status = vaCopySurfaceGLX(m_ctx.display, m_glSurface, id, 0);
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "vaCopySurfaceGLX():" << vaErrorStr(status);
	vaSyncSurface(m_ctx.display, id);
	return status == VA_STATUS_SUCCESS;
#endif
}

bool HwAccel::setBuffer(mp_image_t *mpi) {
#ifdef Q_WS_MAC
	uchar *dummy = reinterpret_cast<uint8_t*>(1);
#endif
#ifdef Q_WS_X11
	if (mpi->type != MP_IMGTYPE_NUMBERED)
		return false;
	if (mpi->imgfmt != IMGFMT_VDPAU)
		return false;
	if (mpi->priv)
		m_usingIds.push_front((VASurfaceID)(uintptr_t)mpi->priv);
	VASurfaceID id = VA_INVALID_SURFACE;
	if (!m_usingIds.isEmpty())
		id = m_usingIds.takeLast();
	else {
		Q_ASSERT(!m_freeIds.isEmpty());
		id = m_freeIds.takeFirst();
	}
	Q_ASSERT(id != VA_INVALID_SURFACE);
	mpi->priv = (void*)(uintptr_t)id;
	uchar *dummy = (uchar*)(uintptr_t)id;
#endif
	mpi->num_planes = 1;
	mpi->flags |= MP_IMGFLAG_DIRECT;
	mpi->stride[0] = mpi->stride[1] = mpi->stride[2] = mpi->stride[3] = 0;
	mpi->planes[0] = mpi->planes[1] = mpi->planes[2] = mpi->planes[3] = nullptr;
	mpi->planes[0] = mpi->planes[3] = dummy;
	return true;
}
