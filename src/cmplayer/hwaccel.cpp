#include "videoframe.hpp"
#include "pref.hpp"
#include "hwaccel.hpp"
#ifdef Q_OS_LINUX
#include "videoframe.hpp"
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

#ifdef Q_OS_LINUX

VADisplay HwAccelInfo::m_display = 0;
QVector<VAProfile> HwAccelInfo::m_profiles;

extern "C" int is_hwaccel_available(AVCodecContext *avctx) {
    if (!cPref.enable_hwaccel || !cPref.hwaccel_codecs.contains(avctx->codec_id))
        return false;
    if (!HwAccelInfo().supports(avctx))
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
    static bool first = true;
	if (first) {
#ifdef Q_OS_LINUX
        auto dpy = XOpenDisplay(NULL);
		if (!(m_display = vaGetDisplay(dpy)))
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
#ifdef Q_OS_LINUX
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
#ifdef Q_OS_LINUX
    int count = 0;
    return find(codec, count) != NoProfile;
#endif
    return false;
}

#ifdef Q_OS_LINUX
VAProfile HwAccelInfo::find(CodecID codec, int &surfaceCount) const {
	if (!m_ok)
		return NoProfile;
    static const QVector<VAProfile> mpeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
    static const QVector<VAProfile> mpeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
    static const QVector<VAProfile> h264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};
    static const QVector<VAProfile> wmv3s = {VAProfileVC1Main, VAProfileVC1Simple};
    static const QVector<VAProfile> vc1s = {VAProfileVC1Advanced};
	auto matched = [this](const QVector<VAProfile> &needs) {
		for (auto need : needs) { if (m_profiles.contains(need)) return need; } return NoProfile;
	};
#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */
    switch(codec) {
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
        surfaceCount = NUM_VIDEO_SURFACES_MPEG2;
		return matched(mpeg2s);
    case AV_CODEC_ID_MPEG4:
        surfaceCount = NUM_VIDEO_SURFACES_MPEG4;
		return matched(mpeg4s);
    case AV_CODEC_ID_WMV3:
        surfaceCount = NUM_VIDEO_SURFACES_VC1;
		return matched(wmv3s);
    case AV_CODEC_ID_VC1:
        surfaceCount = NUM_VIDEO_SURFACES_VC1;
		return matched(vc1s);
    case AV_CODEC_ID_H264:
        surfaceCount = NUM_VIDEO_SURFACES_H264;
		return matched(h264s);
    default:
        break;
    }
    m_avctx = nullptr;
    return NoProfile;
}
#endif

#ifdef Q_OS_LINUX

HwAccel::HwAccel(AVCodecContext *avctx) {
	m_ctx.hwaccel = this;
	m_vaImage.image_id = VA_INVALID_ID;
	HwAccelInfo info;
	if (!info.isAvailable())
		return;
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
	if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
        return;
    m_ctx.ctx.display = info.display();
    m_ctx.ctx.config_id = VA_INVALID_ID;
    m_ctx.ctx.context_id = VA_INVALID_ID;
    if(vaCreateConfig(info.display(), m_profile, VAEntrypointVLD, &attr, 1, &m_ctx.ctx.config_id))
        return;
	if (vaCreateSurfaces(m_ctx.ctx.display, m_width, m_height, VA_RT_FORMAT_YUV420, m_ids.size(), m_ids.data())
			&& vaCreateSurfaces(m_ctx.ctx.display, m_width, m_height, VA_RT_FORMAT_YUV422, m_ids.size(), m_ids.data()))
		return;
    if (vaCreateContext(m_ctx.ctx.display, m_ctx.ctx.config_id, m_width, m_height, VA_PROGRESSIVE, m_ids.data(), m_ids.size(), &m_ctx.ctx.context_id))
        return;
	VAImage vaImage;
	if (vaDeriveImage(info.display(), m_ids[0], &vaImage) != VA_STATUS_SUCCESS) {
		qDebug() << "vaDeriveImage() is not supported! CMPlayer supports vaDeriveImage() only.";
		return;
	}
	vaDestroyImage(info.display(), vaImage.image_id);
    for (auto id : m_ids)
		m_freeIds.push_back(id);
	m_avctx = avctx;
	m_usable = true;
}

HwAccel::~HwAccel() {
	HwAccelInfo info;
	if (!info.isAvailable())
		return;
    if (m_ctx.ctx.display) {
        if (m_ctx.ctx.context_id != VA_INVALID_ID)
            vaDestroyContext(m_ctx.ctx.display, m_ctx.ctx.context_id);
        for (auto id : m_ids) {
            if (id != VA_INVALID_SURFACE)
                vaDestroySurfaces(m_ctx.ctx.display, &id, 1);
        }
        if (m_ctx.ctx.config_id != VA_INVALID_ID)
            vaDestroyConfig(m_ctx.ctx.display, m_ctx.ctx.config_id);
    }
}

bool HwAccel::isCompatibleWith(const AVCodecContext *avctx) const {
	if (!m_usable || m_width != avctx->width || m_height != avctx->height)
		return false;
    HwAccelInfo info;
	int count = 0;
    auto profile = info.find(avctx->codec->id, count);
    return profile != HwAccelInfo::NoProfile && profile == m_profile && m_ids.size() == count;
}

bool HwAccel::copyTo(mp_image_t *mpi, VideoFrame &frame) {
	const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
	vaSyncSurface(m_ctx.ctx.display, id);
	VAStatus status;
	bool ret = false;
	do {
		if ((status = vaDeriveImage(m_ctx.ctx.display, id, &m_vaImage)) != VA_STATUS_SUCCESS)
			break;
		uchar *temp = nullptr;
		if ((status = vaMapBuffer(m_ctx.ctx.display, m_vaImage.buf, (void**)&temp)) != VA_STATUS_SUCCESS)
			break;
		mp_image mp;
		mp.imgfmt = m_vaImage.format.fourcc;
		mp.stride[0] = m_vaImage.pitches[0];
		mp.stride[1] = m_vaImage.pitches[1];
		mp.stride[2] = m_vaImage.pitches[2];
		mp.num_planes = m_vaImage.num_planes;
		mp.w = mp.width = m_vaImage.width;
		mp.h = mp.height = m_vaImage.height;
		mp.planes[0] = temp + m_vaImage.offsets[0];
		mp.planes[1] = temp + m_vaImage.offsets[1];
		mp.planes[2] = temp + m_vaImage.offsets[2];
		ret = frame.copy(&mp);
		if ((status = vaUnmapBuffer(m_ctx.ctx.display, m_vaImage.buf)) != VA_STATUS_SUCCESS)
			break;
		return ret;
	} while (false);
	if (m_vaImage.image_id != VA_INVALID_ID) {
		vaDestroyImage(m_ctx.ctx.display, m_vaImage.image_id);
		m_vaImage.image_id = VA_INVALID_ID;
	}
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "va(DeriveImage|MapBuffer|UnmapBuffer)():" << vaErrorStr(status);
	return ret;
}

bool HwAccel::setBuffer(mp_image_t *mpi) {
    if (mpi->type != MP_IMGTYPE_NUMBERED || mpi->imgfmt != IMGFMT_VDPAU)
        return false;
    if (mpi->priv)
        releaseBuffer(mpi->planes[3]);
	VASurfaceID id = VA_INVALID_SURFACE;
	if (!m_freeIds.isEmpty())
		id = m_freeIds.takeFirst();
	else {
		Q_ASSERT(!m_usingIds.isEmpty());
		id = m_usingIds.takeLast();
	}
	m_usingIds.push_front(id);
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
	const auto id = (VASurfaceID)(uintptr_t)data;
	for (auto it = m_usingIds.begin(); it != m_usingIds.end(); ++it) {
		if (id == *it) {
			m_usingIds.erase(it);
			break;
		}
	}
	m_freeIds.push_back(id);
}
#endif
