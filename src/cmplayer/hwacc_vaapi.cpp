#include "hwacc_vaapi.hpp"
#include "stdafx.hpp"

#ifdef Q_OS_LINUX

extern "C" {
#include <video/decode/dec_video.h>
#include <video/decode/lavc.h>
#include <video/mp_image.h>
#include <mpvcore/av_common.h>
#include <video/mp_image_pool.h>
#include <va/va_glx.h>
#include <va/va_x11.h>
#include <va/va_vpp.h>
#include <video/sws_utils.h>
#include <libavcodec/vaapi.h>
}

bool VaApi::init = false;
VADisplay VaApi::m_display = nullptr;
VaApi &VaApi::get() {static VaApi info; return info;}

VaApiFilterInfo::VaApiFilterInfo(VAContextID context, VAProcFilterType type) {
	m_type = type;
	uint size = 0;
	auto dpy = VaApi::glx();
	switch (type) {
	case VAProcFilterNoiseReduction:
	case VAProcFilterSharpening: {
		VAProcFilterCap cap; size = 1;
		if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, type, &cap, &size)) || size != 1)
			return;
		m_caps.resize(1); m_caps[0].algorithm = type; m_caps[0].range = cap.range;
		break;
	} case VAProcFilterDeinterlacing: {
		size = VAProcDeinterlacingCount;
		VAProcFilterCapDeinterlacing caps[VAProcDeinterlacingCount];
		if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, VAProcFilterDeinterlacing, caps, &size)))
			return;
		m_caps.resize(size);
		for (uint i=0; i<size; ++i)
			m_caps[i].algorithm = caps[i].type;
		break;
	} case VAProcFilterColorBalance: {
		size = VAProcColorBalanceCount;
		VAProcFilterCapColorBalance caps[VAProcColorBalanceCount];
		if (!isSuccess(vaQueryVideoProcFilterCaps(dpy, context, VAProcFilterColorBalance, caps, &size)))
			return;
		m_caps.resize(size);
		for (uint i=0; i<size; ++i) {
			m_caps[i].algorithm = caps[i].type;
			m_caps[i].range = caps[i].range;
		}
		break;
	} default:
		return;
	}
	m_algorithms.resize(m_caps.size());
	for (int i=0; i<m_caps.size(); ++i)
		m_algorithms[i] = m_caps[i].algorithm;
}

struct VaApiSurface {
	VASurfaceID id = VA_INVALID_SURFACE;
	bool ref = false;
	quint64 order = 0;
};

class VaApiSurfacePool : public VaApiStatusChecker {
public:
	VaApiSurfacePool() { memset(&m_null, 0, sizeof(m_null)); }
	~VaApiSurfacePool() { clear(); }

	VAStatus create(int size, int width, int height, uint format) {
		if (m_width == width && m_height == height && m_format == format && m_surfaces.size() == size)
			return isSuccess(VA_STATUS_SUCCESS);
		clear();
		m_width = width;
		m_height = height;
		m_format = format;
		m_ids.resize(size);
		if (!isSuccess(vaCreateSurfaces(VaApi::glx(), width, height, format, size, m_ids.data()))) {
			m_ids.clear();
			return status();
		}
		m_surfaces.resize(size);
		for (int i=0; i<size; ++i)
			m_surfaces[i].id = m_ids[i];
		return status();
	}

	VaApiSurface *getSurface() {
		int i_old, i;
		for (i=0, i_old=0; i<m_surfaces.size(); ++i) {
			if (!m_surfaces[i].ref)
				break;
			if (m_surfaces[i].order < m_surfaces[i_old].order)
				i_old = i;
		}
		if (i >= m_surfaces.size())
			i = i_old;
		auto surface = &m_surfaces[i];
		surface->ref = true;
		surface->order = ++m_order;
		Q_ASSERT(surface->id != VA_INVALID_ID);
		return surface;
	}
	VaApiSurface *getSurface(int index) { return &m_surfaces[index]; }
	mp_image *getMpImage() {
		auto surface = getSurface();
		auto release = [](void *arg) { ((VaApiSurface*)arg)->ref = false; };
		auto mpi = mp_image_new_custom_ref(&m_null, &surface, release);
		mp_image_setfmt(mpi, IMGFMT_VAAPI);
		mp_image_set_size(mpi, m_width, m_height);
		mpi->planes[1] = mpi->planes[2] = nullptr;
		mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)surface->id;
		return mpi;
	}
	void clear() {
		for (auto &it : m_surfaces) {
			if (it.id != VA_INVALID_SURFACE)
				vaDestroySurfaces(VaApi::glx(), &it.id, 1);
		}
		m_surfaces.clear();
		m_ids.clear();
	}
	QVector<VASurfaceID> ids() const {return m_ids;}
	uint format() const {return m_format;}
private:
	QVector<VASurfaceID> m_ids;
	QVector<VaApiSurface> m_surfaces;
	uint m_format = 0;
	int m_width = 0, m_height = 0;
	quint64 m_order = 0LL;
	mp_image m_null;
};

VaApi::VaApi() {
	init = true;
	auto xdpy = QX11Info::display();
	VADisplay display = m_display = vaGetDisplayGLX(xdpy);
	if (!display)
		return;
	int major, minor;
	if (!isSuccess(vaInitialize(m_display, &major, &minor)))
		return;
	auto size = vaMaxNumProfiles(display);
	QVector<VAProfile> profiles;
	profiles.resize(size);
	if (vaQueryConfigProfiles(display, profiles.data(), &size) != VA_STATUS_SUCCESS)
		return;
	profiles.resize(size);

	for (auto profile : profiles) {
		int size = vaMaxNumEntrypoints(display);
		QVector<VAEntrypoint> entries(size, VAEntrypointMax);
		if (vaQueryConfigEntrypoints(display, profile, entries.data(), &size) != VA_STATUS_SUCCESS)
			continue;
		entries.resize(size);
		m_entries.insert(profile, entries);
	}

	auto supports = [this, &profiles](const QVector<VAProfile> &va_all, const QVector<int> &av_all, int surfaces, AVCodecID id) {
		QVector<VAProfile> va; QVector<int> av;
		for (int i=0; i<va_all.size(); ++i) {
			if (hasEntryPoint(VAEntrypointVLD, va_all[i])) {
				va.push_back(va_all[i]);
				av.push_back(av_all[i]);
			}
		}
		if (!va.isEmpty())
			m_supported.insert(id, VaApiCodec(profiles, va, av, surfaces, id));
	};
#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */
	const QVector<VAProfile> vampeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
	const QVector<int> avmpeg2s = {FF_PROFILE_MPEG2_MAIN, FF_PROFILE_MPEG2_SIMPLE};
	const QVector<VAProfile> vampeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
	const QVector<int> avmpeg4s = {FF_PROFILE_MPEG4_MAIN, FF_PROFILE_MPEG4_ADVANCED_SIMPLE, FF_PROFILE_MPEG4_SIMPLE};
	const QVector<VAProfile> vah264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline, VAProfileH264ConstrainedBaseline};
	const QVector<int> avh264s = {FF_PROFILE_H264_HIGH, FF_PROFILE_H264_MAIN, FF_PROFILE_H264_BASELINE, FF_PROFILE_H264_CONSTRAINED_BASELINE};
	const QVector<VAProfile> vawmv3s = {VAProfileVC1Main, VAProfileVC1Simple, VAProfileVC1Advanced};
	const QVector<int> avwmv3s = {FF_PROFILE_VC1_MAIN, FF_PROFILE_VC1_SIMPLE, FF_PROFILE_VC1_ADVANCED};
	const QVector<VAProfile> vavc1s = {VAProfileVC1Advanced, VAProfileVC1Main, VAProfileVC1Simple};
	const QVector<int> avvc1s = {FF_PROFILE_VC1_ADVANCED, FF_PROFILE_VC1_MAIN, FF_PROFILE_VC1_SIMPLE};
	supports(vampeg2s, avmpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG1VIDEO);
	supports(vampeg2s, avmpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG2VIDEO);
	supports(vampeg4s, avmpeg4s, NUM_VIDEO_SURFACES_MPEG4, AV_CODEC_ID_MPEG4);
	supports(vawmv3s, avwmv3s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_WMV3);
	supports(vavc1s, avvc1s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_VC1);
	supports(vah264s, avh264s, NUM_VIDEO_SURFACES_H264, AV_CODEC_ID_H264);

	if (hasEntryPoint(VAEntrypointVideoProc, VAProfileNone)) {
		VAConfigID config = VA_INVALID_ID;
		VAContextID context = VA_INVALID_ID;
		do {
			if (!isSuccess(vaCreateConfig(display, VAProfileNone, VAEntrypointVideoProc, nullptr, 0, &config)))
				break;
			if (!isSuccess(vaCreateContext(display, config, 0, 0, 0, nullptr, 0, &context)))
				break;
			QVector<VAProcFilterType> types(VAProcFilterCount);
			uint size = VAProcFilterCount;
			if (!isSuccess(vaQueryVideoProcFilters(display, context, types.data(), &size)))
				break;
			types.resize(size);
			for (const auto &type : types) {
				auto info = VaApiFilterInfo(context, type);
				if (info.isSuccess())
					m_filters.insert(type, info);
			}
		} while (false);
		if (context != VA_INVALID_ID)
			vaDestroyContext(display, context);
		if (config != VA_INVALID_ID)
			vaDestroyConfig(display, config);
	}
}

void VaApi::finalize() {
	auto close = [] (VADisplay &dpy) { if (dpy) { vaTerminate(dpy); dpy = nullptr; } };
	close(m_display);
	init = false;
}

void initialize_vaapi() {
	if (!VaApi::init)
		VaApi::get().glx();
}

void finalize_vaapi() {
	if (VaApi::init)
		VaApi::get().finalize();
}

/***************************************************************************************************/

struct HwAccVaApi::Data {
	vaapi_context context = {nullptr, VA_INVALID_ID, VA_INVALID_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	VAProfile profile = VAProfileNone;
	VaApiSurfacePool pool;
};

HwAccVaApi::HwAccVaApi(AVCodecID cid)
: HwAcc(cid), d(new Data) {
}

HwAccVaApi::~HwAccVaApi() {
	freeContext();
	if (d->context.config_id != VA_INVALID_ID)
		vaDestroyConfig(d->context.display, d->context.config_id);
	delete d;
}

bool HwAccVaApi::isOk() const {
	return status() == VA_STATUS_SUCCESS;
}

void *HwAccVaApi::context() const {
	return &d->context;
}

mp_image *HwAccVaApi::getSurface() {
	return d->pool.getMpImage();
}

void HwAccVaApi::freeContext() {
	if (d->context.display) {
		if (d->context.context_id != VA_INVALID_ID)
			vaDestroyContext(d->context.display, d->context.context_id);
	}
	d->context.context_id = VA_INVALID_ID;
}

bool HwAccVaApi::fillContext(AVCodecContext *avctx) {
	if (status() != VA_STATUS_SUCCESS)
		return false;
	freeContext();
	d->context.display = VaApi::glx();
	if (!d->context.display)
		return false;
	const auto codec = VaApi::codec(avctx->codec_id);
	if (!codec)
		return false;
	d->profile = codec->profile(avctx->profile);
	VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
	if(!isSuccess(vaGetConfigAttributes(d->context.display, d->profile, VAEntrypointVLD, &attr, 1)))
		return false;
	if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
		return isSuccess(VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT);
	if(!isSuccess(vaCreateConfig(d->context.display, d->profile, VAEntrypointVLD, &attr, 1, &d->context.config_id)))
		return false;
	const int w = avctx->width, h = avctx->height;
	auto format = VA_RT_FORMAT_YUV420;
	if (!isSuccess(d->pool.create(codec->surfaces, w, h, format))) {
		if (!isSuccess(d->pool.create(codec->surfaces, w, h, format = VA_RT_FORMAT_YUV422)))
			return false;
	}
	VaApi::get().setSurfaceFormat(format);
	auto ids = d->pool.ids();
	if (!isSuccess(vaCreateContext(d->context.display, d->context.config_id, w, h, VA_PROGRESSIVE, ids.data(), ids.size(), &d->context.context_id)))
		return false;
	return true;
}

mp_image *HwAccVaApi::getImage(mp_image *mpi) {
	return mpi;
}

/*************************************************************************************************/

//struct VaApiImage {
//	VaApiImage(VAImageFormat *format, const QSize &size) {
//		this->format = format;
//		vaCreateImage(display, format, size.width(), size.height(), &image);
//		Q_ASSERT(format->fourcc == image.format.fourcc);
//	}
//	~VaApiImage() {
//		if (image.image_id != VA_INVALID_ID)
//			vaDestroyImage(display, image.image_id);
//	}
//	VAImage image;
//	bool ref = false;
//	VADisplay display = VaApi::x11();
//	VAImageFormat *format = nullptr;
//};

//typedef QLinkedList<VaApiImage*> ImagePool;
//struct HwAccVaApiX11::Data {
//	quint32 fourcc = 0;
//	mp_imgfmt imgfmt = IMGFMT_NONE;
//	ImagePool pool;
//	ImagePool::iterator it;
//	VAImageFormat format;
//};

//HwAccVaApiX11::HwAccVaApiX11(AVCodecID codec)
//: HwAccVaApi(codec, VaApiX11), d(new Data) {
//	d->it = d->pool.end();
//}

//HwAccVaApiX11::~HwAccVaApiX11() {
//	for (auto image : d->pool) {
//		image->format = nullptr;
//		if (!image->ref)
//			delete image;
//	}
//	delete d;
//}

//bool HwAccVaApiX11::fillContext(AVCodecContext *avctx) {
//	if (!HwAccVaApi::fillContext(avctx))
//		return false;
//	VAImage image;
//	auto destroy = [&image, this] () {
//		if (image.image_id != VA_INVALID_ID) {
//			vaDestroyImage(vaapi()->display, image.image_id);
//			image.image_id = VA_INVALID_ID;
//		}
//	};
//	auto findFormat =[&image, &destroy, avctx, this] () {
//		int count = vaMaxNumImageFormats(vaapi()->display);
//		QVector<VAImageFormat> formats;
//		formats.resize(count);
//		vaQueryImageFormats(vaapi()->display, formats.data(), &count);
//		formats.resize(count);
//		for (int i=0; i<formats.size(); ++i) {
//			if (!isSuccess(vaCreateImage(vaapi()->display, &formats[i], avctx->width, avctx->height, &image))) {
//				image.image_id = VA_INVALID_ID;
//				continue;
//			}
//			if (!isSuccess(vaGetImage(vaapi()->display, static_cast<VaApiSurface*>(surface(0))->id, 0, 0, avctx->width, avctx->height, image.image_id))) {
//				destroy();
//				continue;
//			}
//			d->format = formats[i];
//			switch (image.format.fourcc) {
//			case VA_FOURCC_NV12:
//				return IMGFMT_NV12;
//			case VA_FOURCC_YV12:
//			case VA_FOURCC('I', '4', '2', '0'):
//				return IMGFMT_420P;
//			case VA_FOURCC_YUY2:
//				return IMGFMT_YUYV;
//			case VA_FOURCC_UYVY:
//				return IMGFMT_UYVY;
//			default:
//				status() = VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
//				break;
//			}
//		}
//		return IMGFMT_NONE;
//	};
//	d->imgfmt = findFormat();
//	d->fourcc = image.format.fourcc;
//	destroy();
//	return isOk();
//}

//VaApiImage *HwAccVaApiX11::newImage() {
//	if (!d->pool.isEmpty()) {
//		for (auto it = d->it; it != d->pool.end(); ++it) {
//			if (!(*it)->ref)
//				return *(d->it = it);
//		}
//		for (auto it = d->pool.begin(); it != d->it; ++it) {
//			if (!(*it)->ref)
//				return *(d->it = it);
//		}
//	}
//	return *(d->it = d->pool.insert(d->pool.end(), new VaApiImage(&d->format, size())));
//}

//mp_image *HwAccVaApiX11::getImage(mp_image *mpi) {
//	if (mpi->fmt.id != IMGFMT_VAAPI)
//		return nullptr;
//	const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
//	vaSyncSurface(vaapi()->display, id);
//	auto img = newImage();
//	vaGetImage(vaapi()->display, id, 0, 0, size().width(), size().height(), img->image.image_id);
//	void *data = nullptr;
//	vaMapBuffer(vaapi()->display, img->image.buf, &data);
//	img->ref = true;
//	auto release = [](void *data) {
//		auto img = static_cast<VaApiImage*>(data);
//		vaUnmapBuffer(img->display, img->image.buf);
//		img->ref = false;
//		if (!img->format)
//			delete img;
//	};
//	mpi = nullImage(d->imgfmt, img->image.width, img->image.height, img, release);
//	uint offsets[3] = {img->image.offsets[0], img->image.offsets[1], img->image.offsets[2]};
//	if (img->image.format.fourcc == VA_FOURCC_YV12)
//		qSwap(offsets[1], offsets[2]);
//	for (uint i=0; i<img->image.num_planes; ++i) {
//		mpi->stride[i] = img->image.pitches[i];
//		mpi->planes[i] = (uchar*)data + offsets[i];
//	}
//	return mpi;
//}

#else
void initialize_vaapi() {}
void finalize_vaapi() {}
#endif

