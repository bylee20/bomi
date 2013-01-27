#include "pref.hpp"
#include "hwaccel.hpp"

extern "C" int is_hwaccel_usable(CodecID codec) {
	return cPref.enable_hwaccel && cPref.hwaccel_codecs.contains(codec) && HwAccel::supports(codec);
}

QList<AVCodecID> HwAccel::fullCodecList() {
	return QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO << AV_CODEC_ID_MPEG2VIDEO << AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3 << AV_CODEC_ID_VC1 << AV_CODEC_ID_H264;
}

#ifdef Q_OS_MAC
HwAccel::HwAccel() { m_init = true; }
bool HwAccel::supports(AVCodecID codec) { return codec == AV_CODEC_ID_H264; }
bool HwAccel::set(AVCodecContext *avctx) { return m_on = (avctx->codec && !strcmp(avctx->codec->name, "h264_vda")); }
#endif

#ifdef Q_OS_LINUX
#include "videoframe.hpp"

HwAccel::Info &HwAccel::Info::get() { static Info info; return info; }

extern "C" {
static void (*vd_ffmpeg_release_buffer)(AVCodecContext *, AVFrame*) = nullptr;
void hwaccel_release_buffer(AVCodecContext *avctx, AVFrame *frame) {
	mp_image_t *mpi = static_cast<mp_image_t*>(avctx->opaque);
	if (mpi->priv && avctx->hwaccel_context)
		static_cast<HwAccel::Context*>(avctx->hwaccel_context)->hwaccel->releaseBuffer(frame->data[3]);
	if (vd_ffmpeg_release_buffer)
		vd_ffmpeg_release_buffer(avctx, frame);
	else
		frame->data[0] = frame->data[1] = frame->data[2] = frame->data[3] = nullptr;
}
int register_hwaccel_callbacks(AVCodecContext *avctx) {
	vd_ffmpeg_release_buffer = avctx->release_buffer;
	avctx->release_buffer = hwaccel_release_buffer;
	return false;
}
}

HwAccel::Info::Info() {
	do {
		if (!(xdpy = XOpenDisplay(NULL)))
			break;
		if (!(display = vaGetDisplay(xdpy)))
			break;
		int major, minor;
		if (vaInitialize(display, &major, &minor))
			break;
		auto size = vaMaxNumProfiles(display);
		profiles.resize(size);
		if (vaQueryConfigProfiles(display, profiles.data(), &size))
			break;
		profiles.resize(size);
		ok = true;

		auto supports = [this](const QVector<VAProfile> &candidates, int count) {
			ProfileInfo info;
			for (auto one : candidates) {
				if (profiles.contains(one))
					info.profiles.push_back(one);
			}
			if (!info.profiles.isEmpty())
				info.surfaceCount = count;
			return info;
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
		supported[AV_CODEC_ID_MPEG1VIDEO] = supported[AV_CODEC_ID_MPEG2VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2);
		supported[AV_CODEC_ID_MPEG4] = supports(mpeg4s, NUM_VIDEO_SURFACES_MPEG4);
		supported[AV_CODEC_ID_WMV3] = supports(wmv3s, NUM_VIDEO_SURFACES_VC1);
		supported[AV_CODEC_ID_VC1] = supports(vc1s, NUM_VIDEO_SURFACES_VC1);
		supported[AV_CODEC_ID_H264] = supports(h264s, NUM_VIDEO_SURFACES_H264);
	} while (0);
	if (!ok)
		profiles.clear();
}

void HwAccel::finalize() {
	auto &info = Info::get();
	if (info.display)
		vaTerminate(info.display);
	if (info.xdpy)
		XCloseDisplay(info.xdpy);
}

bool HwAccel::supports(AVCodecID codec) { return find(codec) != nullptr; }

const HwAccel::ProfileInfo *HwAccel::find(AVCodecID codec) {
	const auto &info = Info::get();
	auto it = info.supported.find(codec);
	return it != info.supported.end() && !it->profiles.isEmpty() ? &(*it) : nullptr;
}

HwAccel::HwAccel() {
	memset(&m_ctx.ctx, 0, sizeof(m_ctx.ctx));
	m_ctx.hwaccel = this;
	m_ctx.ctx.config_id = m_ctx.ctx.context_id = m_vaImage.image_id = VA_INVALID_ID;
	m_init = Info::get().ok && (m_ctx.ctx.display = Info::get().display);
}

void HwAccel::free() {
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
	m_ctx.ctx.context_id = m_ctx.ctx.config_id = VA_INVALID_ID;
	m_profile = (VAProfile)(-1);
	m_ids.clear();
	m_freeIds.clear();
	m_usingIds.clear();
}

bool HwAccel::set(AVCodecContext *avctx) {
	avctx->hwaccel_context = nullptr;
	m_usable = false;
	if (!m_init)
		return false;
	auto info = find(avctx->codec_id);
	if (!info)
		return false;
	if (m_width != avctx->width || m_height != avctx->height || info->profiles.front() != m_profile) {
		free();
		m_width = avctx->width; m_height = avctx->height;
		m_profile = info->profiles.front();
		if (m_width <= 0 || m_height <= 0)
			return false;
		VAStatus status = VA_STATUS_SUCCESS;
		do {
			VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
			if((status = vaGetConfigAttributes(m_ctx.ctx.display, m_profile, VAEntrypointVLD, &attr, 1)) != VA_STATUS_SUCCESS)
				break;
			if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
				{ status = VA_STATUS_ERROR_FLAG_NOT_SUPPORTED; break; }
			if((status = vaCreateConfig(m_ctx.ctx.display, m_profile, VAEntrypointVLD, &attr, 1, &m_ctx.ctx.config_id)) != VA_STATUS_SUCCESS)
				break;
			m_ids = QVector<VASurfaceID>(info->surfaceCount, VA_INVALID_SURFACE);
			if ((status =vaCreateSurfaces(m_ctx.ctx.display, m_width, m_height, VA_RT_FORMAT_YUV420, m_ids.size(), m_ids.data())) != VA_STATUS_SUCCESS
					&& (status = vaCreateSurfaces(m_ctx.ctx.display, m_width, m_height, VA_RT_FORMAT_YUV422, m_ids.size(), m_ids.data())) != VA_STATUS_SUCCESS)
				break;
			if ((status = vaCreateContext(m_ctx.ctx.display, m_ctx.ctx.config_id, m_width, m_height, VA_PROGRESSIVE, m_ids.data(), m_ids.size(), &m_ctx.ctx.context_id)) != VA_STATUS_SUCCESS)
				break;
			VAImage vaImage;
			if ((status = vaDeriveImage(m_ctx.ctx.display, m_ids[0], &vaImage)) != VA_STATUS_SUCCESS)
				break;
			vaDestroyImage(m_ctx.ctx.display, vaImage.image_id);
			for (auto id : m_ids)
				m_freeIds.push_back(id);
		} while(0);
		if (status != VA_STATUS_SUCCESS) {
			qDebug() << "vaapi:" << vaErrorStr(status);
			return false;
		}
	}
	avctx->hwaccel_context = &m_ctx.ctx;
	m_usable = true;
	return true;
}

mp_image &HwAccel::extract(mp_image *mpi) {
	const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
	vaSyncSurface(m_ctx.ctx.display, id);
	VAStatus status;
	memset(&m_mpi, 0, sizeof(m_mpi));
	do {
		if ((status = vaDeriveImage(m_ctx.ctx.display, id, &m_vaImage)) != VA_STATUS_SUCCESS)
			break;
		uchar *temp = nullptr;
		if ((status = vaMapBuffer(m_ctx.ctx.display, m_vaImage.buf, (void**)&temp)) != VA_STATUS_SUCCESS)
			break;
		m_mpi.imgfmt = m_vaImage.format.fourcc;
		m_mpi.stride[0] = m_vaImage.pitches[0];
		m_mpi.stride[1] = m_vaImage.pitches[1];
		m_mpi.stride[2] = m_vaImage.pitches[2];
		m_mpi.num_planes = m_vaImage.num_planes;
		m_mpi.w = m_mpi.width = m_vaImage.width;
		m_mpi.h = m_mpi.height = m_vaImage.height;
		m_mpi.planes[0] = temp + m_vaImage.offsets[0];
		m_mpi.planes[1] = temp + m_vaImage.offsets[1];
		m_mpi.planes[2] = temp + m_vaImage.offsets[2];
	} while (0);
	if (status != VA_STATUS_SUCCESS)
		qDebug() << "va(DeriveImage|MapBuffer)():" << vaErrorStr(status);
	return m_mpi;
}

void HwAccel::clean() {
	VAStatus status;
	if ((status = vaUnmapBuffer(m_ctx.ctx.display, m_vaImage.buf)) != VA_STATUS_SUCCESS)
		qDebug() << "va(UnmapBuffer)():" << vaErrorStr(status);
	if (m_vaImage.image_id != VA_INVALID_ID) {
		vaDestroyImage(m_ctx.ctx.display, m_vaImage.image_id);
		m_vaImage.image_id = VA_INVALID_ID;
	}
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
