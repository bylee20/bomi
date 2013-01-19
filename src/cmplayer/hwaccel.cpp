#include "videoframe.hpp"
#include "pref.hpp"
#include "hwaccel.hpp"
#ifdef Q_OS_X11
#include <QtGui/QX11Info>
#endif

#ifdef Q_OS_MAC
extern "C" int is_hwaccel_available(const char *name) {
	if (!cPref.enable_hwaccel)
		return false;
	AVCodecID codec = AV_CODEC_ID_NONE;
	if (strstr(name, "ffh264vda"))
		codec = AV_CODEC_ID_H264;
	if (!cPref.hwaccel_codecs.contains(codec) || !HwAccelInfo().supports(codec))
		return false;
	qDebug() << "HwAccel is available!";
	return true;
}
#endif

#ifdef Q_OS_X11
extern "C" int is_hwaccel_available(AVCodecContext *avctx) {
	if (!cPref.enable_hwaccel || !cPref.hwaccel_codecs.contains(avctx->codec_id))
		return false;
	if (!HwAccelInfo::get().supports(avctx))
		return false;
	qDebug() << "HwAccel is available!";
	return true;
}

static void (*vd_ffmpeg_release_buffer)(AVCodecContext *, AVFrame*) = nullptr;

void hwaccel_release_buffer(AVCodecContext *avctx, AVFrame *frame) {
	mp_image_t *mpi = reinterpret_cast<mp_image_t*>(avctx->opaque);
	if (mpi->priv && avctx->hwaccel_context)
		reinterpret_cast<HwAccel::Context*>(avctx->hwaccel_context)->hwaccel->releaseBuffer(frame->data[3]);
	if (vd_ffmpeg_release_buffer)
		vd_ffmpeg_release_buffer(avctx, frame);
	else
		frame->data[0] = frame->data[1] = frame->data[2] = frame->data[3] = nullptr;
}

extern "C" int register_hwaccel_callbacks(AVCodecContext *avctx) {
	vd_ffmpeg_release_buffer = avctx->release_buffer;
	avctx->release_buffer = hwaccel_release_buffer;
	return false;
}

#endif

AVCodecContext *HwAccelInfo::m_avctx = nullptr;
bool HwAccelInfo::m_ok = false;

HwAccelInfo::HwAccelInfo() {
	static bool first = false;
	if (first) {
#ifdef Q_OS_X11
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
}

void HwAccelInfo::finalize() {
#ifdef Q_OS_X11
	if (m_display)
		vaTerminate(m_display);
#endif
}

QList<AVCodecID> HwAccelInfo::fullCodecList() const {
	return QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO
		<< AV_CODEC_ID_MPEG2VIDEO
		<< AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3
		<< AV_CODEC_ID_VC1
		<< AV_CODEC_ID_H264;
}

bool HwAccelInfo::supports(AVCodecID codec) const {
#ifdef Q_OS_MAC
	return codec == AV_CODEC_ID_H264;
#endif
#ifdef Q_OS_X11
	int count = 0;
	return find(codec, count) != NoProfile;
#endif
}

#ifdef Q_OS_X11
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
	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		surfaceCount = NUM_VIDEO_SURFACES_MPEG2;
		return findMatchedProfile(mpeg2s);
	case AV_CODEC_ID_MPEG4:
		surfaceCount = NUM_VIDEO_SURFACES_MPEG4;
		return findMatchedProfile(mpeg4s);
	case AV_CODEC_ID_WMV3:
		surfaceCount = NUM_VIDEO_SURFACES_VC1;
		return findMatchedProfile(wmv3s);
	case AV_CODEC_ID_VC1:
		surfaceCount = NUM_VIDEO_SURFACES_VC1;
		return findMatchedProfile(vc1s);
	case AV_CODEC_ID_H264:
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
#endif

HwAccel::HwAccel(AVCodecContext *avctx) {
	m_ctx.hwaccel = this;
	HwAccelInfo info;
	if (!info.isAvailable())
		return;
#ifdef Q_OS_X11
	memset(&m_ctx.ctx, 0, sizeof(m_ctx.ctx));
	m_width = avctx->width;
	m_height = avctx->height;
	if (m_width <= 0 || m_height <= 0)
		return;
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

	m_ctx.ctx.display = info.display();
	m_ctx.ctx.config_id = VA_INVALID_ID;
	m_ctx.ctx.context_id = VA_INVALID_ID;
	if(vaCreateConfig(info.display(), m_profile, VAEntrypointVLD, &attr, 1, &m_ctx.ctx.config_id))
		return;
	if (vaCreateSurfaces(m_ctx.ctx.display, m_width, m_height, VA_RT_FORMAT_YUV420, m_ids.size(), m_ids.data()))
		return;
	if (vaCreateContext(m_ctx.ctx.display, m_ctx.ctx.config_id, m_width, m_height, VA_PROGRESSIVE, m_ids.data(), m_ids.size(), &m_ctx.ctx.context_id))
		return;
	for (auto id : m_ids)
		m_freeIds.push_back(id);
#endif
	m_avctx = avctx;
	m_usable = true;
}

HwAccel::~HwAccel() {
	HwAccelInfo info;
	if (!info.isAvailable())
		return;
#ifdef Q_OS_X11
	if (m_ctx.ctx.display) {
		if (m_glSurface)
			vaDestroySurfaceGLX(m_ctx.ctx.display, &m_glSurface);
		if (m_ctx.ctx.context_id != VA_INVALID_ID)
			vaDestroyContext(m_ctx.ctx.display, m_ctx.ctx.context_id);
		for (auto id : m_ids) {
			if (id != VA_INVALID_SURFACE)
				vaDestroySurfaces(m_ctx.ctx.display, &id, 1);
		}
		if (m_ctx.ctx.config_id != VA_INVALID_ID)
			vaDestroyConfig(m_ctx.ctx.display, m_ctx.ctx.config_id);
	}
#endif
}

//VideoFormat HwAccel::format() const {
//#ifdef Q_OS_MAC
//	return VideoFormat::fromType(VideoFormat::YV12, m_width, m_height);
//#endif
//#ifdef Q_OS_X11
//	auto format = VideoFormat::fromType(VideoFormat::BGRA, m_width, m_height);
//	format.width_stride = format.stride = m_width;
//	return format;
//#endif
//}

bool HwAccel::isCompatibleWith(const AVCodecContext *avctx) const {
	if (!m_usable || m_width != avctx->width || m_height != avctx->height)
		return false;
#ifdef Q_OS_MAC
	return false;
#endif
#ifdef Q_OS_X11
	int count = 0;
	auto profile = HwAccelInfo::get().find(avctx->codec->id, count);
	return profile != HwAccelInfo::NoProfile && profile == m_profile && m_ids.size() == count;
#endif
}

#ifdef Q_OS_X11
bool HwAccel::createSurface(GLuint *textures) {
	m_textures = textures;
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	int status = vaCreateSurfaceGLX(m_ctx.ctx.display, GL_TEXTURE_2D, m_textures[0], &m_glSurface);
	qDebug() << "vaCreatteSurfaceGLX():" << vaErrorStr(status);
	return status == VA_STATUS_SUCCESS;
}

bool HwAccel::copySurface(mp_image_t *mpi) {
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
	const auto status = vaCopySurfaceGLX(m_ctx.ctx.display, m_glSurface, id, 0);
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "vaCopySurfaceGLX():" << vaErrorStr(status);
	vaSyncSurface(m_ctx.ctx.display, id);
	return status == VA_STATUS_SUCCESS;
}

bool HwAccel::setBuffer(mp_image_t *mpi) {
	if (mpi->type != MP_IMGTYPE_NUMBERED || mpi->imgfmt != IMGFMT_VDPAU)
		return false;
	if (mpi->priv)
		releaseBuffer(mpi->planes[3]);
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
	mpi->num_planes = 1;
	mpi->flags |= MP_IMGFLAG_DIRECT;
	mpi->stride[0] = mpi->stride[1] = mpi->stride[2] = mpi->stride[3] = 0;
	mpi->planes[0] = mpi->planes[1] = mpi->planes[2] = mpi->planes[3] = nullptr;
	mpi->priv = mpi->planes[0] = mpi->planes[3] = dummy;
	return true;
}

void HwAccel::releaseBuffer(void *data) {
	Q_UNUSED(data);
	m_usingIds.push_front((VASurfaceID)(uintptr_t)data);
}
#endif
