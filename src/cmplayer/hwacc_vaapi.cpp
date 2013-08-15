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
#include <video/sws_utils.h>
#if VA_CHECK_VERSION(0, 34, 0)
#include <va/va_compat.h>
#endif
#include <libavcodec/vaapi.h>
}

bool VaApi::init = false;
VaApi &VaApi::get() {static VaApi info; return info;}

VaApi::VaApi() {
	init = true;
	auto dpy = QX11Info::display();
	m_x11Display = vaGetDisplay(dpy);
	m_glxDisplay = vaGetDisplayGLX(dpy);
	VADisplay display = nullptr; int major, minor;
	if (m_x11Display) {
		if (vaInitialize(m_x11Display, &major, &minor) == VA_STATUS_SUCCESS)
			display = m_x11Display;
	}
	if (m_glxDisplay) {
		if (vaInitialize(m_glxDisplay, &major, &minor) == VA_STATUS_SUCCESS)
			display = m_glxDisplay;
	}
	if (!display)
		return;
	auto size = vaMaxNumProfiles(display);
	QVector<VAProfile> profiles;
	profiles.resize(size);
	if (vaQueryConfigProfiles(display, profiles.data(), &size) != VA_STATUS_SUCCESS)
		return;
	profiles.resize(size);
	auto supports = [this, &profiles](const QVector<VAProfile> &va, const QVector<int> &av, int count, AVCodecID id) {
		m_supported.insert(id, VaApiCodec(profiles, va, av, count, id));
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
}

void VaApi::finalize() {
	auto close = [] (VADisplay &dpy) { if (dpy) { vaTerminate(dpy); dpy = nullptr; } };
	close(m_glxDisplay); close(m_x11Display);
	init = false;
}

void initialize_vaapi() {
	if (!VaApi::init)
		VaApi::get().x11();
}

void finalize_vaapi() {
	if (VaApi::init)
		VaApi::get().finalize();
}

/**********************************************************************************************/

VaApiSurfaceGLX::VaApiSurfaceGLX(GLuint texture) {
	m_status = vaCreateSurfaceGLX(VaApi::glx(), GL_TEXTURE_2D, texture, &m_surface);
	if (isValid())
		m_texture = texture;
	else
		m_surface = nullptr;
}

VaApiSurfaceGLX::~VaApiSurfaceGLX() {
	if (m_surface)
		vaDestroySurfaceGLX(VaApi::glx(), m_surface);
}

bool VaApiSurfaceGLX::copy(const uchar *data) {
	const auto id = (VASurfaceID)(uintptr_t)data;
	vaSyncSurface(VaApi::glx(), id);
	m_status = vaCopySurfaceGLX(VaApi::glx(), m_surface, id, VA_SRC_BT601);
	return isValid();
}

/***************************************************************************************************/


struct VaApiSurface { VASurfaceID  id = VA_INVALID_ID; bool ref = false; quint64 order = 0; };

struct HwAccVaApi::Data {
	vaapi_context context = {nullptr, VA_INVALID_ID, VA_INVALID_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	QVector<VaApiSurface> surfaces;
	quint64 surfaceOrder = 0;
	VAProfile profile = VAProfileNone;
};

HwAccVaApi::HwAccVaApi(AVCodecID cid, Type type)
: HwAcc(cid), d(new Data), m_status(VA_STATUS_SUCCESS), m_type(type) {
}

HwAccVaApi::~HwAccVaApi() {
	freeContext();
	if (d->context.config_id != VA_INVALID_ID)
		vaDestroyConfig(d->context.display, d->context.config_id);
	delete d;
}

void *HwAccVaApi::surface(int i) const {
	return &d->surfaces[i];
}

vaapi_context *HwAccVaApi::vaapi() const {
	return &d->context;
}

bool HwAccVaApi::isOk() const {
	return status() == VA_STATUS_SUCCESS;
}

bool HwAccVaApi::isAvailable(AVCodecID codec) const {
	return VaApi::find(codec) != nullptr;
}

void *HwAccVaApi::context() const {
	return &d->context;
}

mp_image *HwAccVaApi::getSurface() {
	int i_old, i;
	for (i=0, i_old=0; i<d->surfaces.size(); ++i) {
		if (!d->surfaces[i].ref)
			break;
		if (d->surfaces[i].order < d->surfaces[i_old].order)
			i_old = i;
	}
	if (i >= d->surfaces.size())
		i = i_old;
	d->surfaces[i].ref = true;
	d->surfaces[i].order = ++d->surfaceOrder;
	Q_ASSERT(d->surfaces[i].id != VA_INVALID_ID);
	auto release = [](void *ref) { *(bool*)ref = false; };
	auto mpi = nullImage(IMGFMT_VAAPI, size().width(), size().height(), &d->surfaces[i].ref, release);
	mpi->planes[1] = mpi->planes[2] = nullptr;
	mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)d->surfaces[i].id;
	return mpi;
}

void HwAccVaApi::freeContext() {
	if (d->context.display) {
		if (d->context.context_id != VA_INVALID_ID)
			vaDestroyContext(d->context.display, d->context.context_id);
		for (auto &surface : d->surfaces) {
			if (surface.id != VA_INVALID_SURFACE)
				vaDestroySurfaces(d->context.display, &surface.id, 1);
			surface = VaApiSurface();
		}
	}
	d->context.context_id = VA_INVALID_ID;
	d->surfaceOrder = 0;
}

bool HwAccVaApi::isSuccess(int result) {
	return (status() = result) == VA_STATUS_SUCCESS;
}

bool HwAccVaApi::fillContext(AVCodecContext *avctx) {
	if (status() != VA_STATUS_SUCCESS)
		return false;
	freeContext();
	d->context.display = VaApi::display(type());
	if (!d->context.display)
		return false;
	const auto codec = VaApi::find(avctx->codec_id);
	if (!codec)
		return false;
	d->profile = codec->profile(avctx->profile);
	d->surfaces.resize(codec->surfaces);
	VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
	if((status() = vaGetConfigAttributes(d->context.display, d->profile, VAEntrypointVLD, &attr, 1)) != VA_STATUS_SUCCESS)
		return false;
	if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
		{status() = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT; return false; }
	if((status() = vaCreateConfig(d->context.display, d->profile, VAEntrypointVLD, &attr, 1, &d->context.config_id)) != VA_STATUS_SUCCESS)
		return false;
	QVector<VASurfaceID> ids(d->surfaces.size(), VA_INVALID_SURFACE);
	const int w = avctx->width, h = avctx->height;
	if (!isSuccess(vaCreateSurfaces(d->context.display, w, h, VA_RT_FORMAT_YUV420, ids.size(), ids.data()))
			&& !isSuccess(vaCreateSurfaces(d->context.display, w, h, VA_RT_FORMAT_YUV422, ids.size(), ids.data())))
		return false;
	for (int i=0; i<ids.size(); ++i)
		d->surfaces[i].id = ids[i];
	if (!isSuccess(vaCreateContext(d->context.display, d->context.config_id, w, h, VA_PROGRESSIVE, ids.data(), ids.size(), &d->context.context_id)))
		return false;
	m_size = QSize(w, h);
	return true;
}

bool HwAccVaApi::check(AVCodecContext *avctx) {
	if (avctx->pix_fmt != AV_PIX_FMT_VAAPI_VLD || !HwAccVaApi::isOk())
		return false;
	if (size().width() == avctx->width && size().height() == avctx->height)
		return true;
	if (!fillContext(avctx))
		return false;
	return true;
}

/*************************************************************************************************/

struct VaApiImage {
	VaApiImage(VAImageFormat *format, const QSize &size) {
		this->format = format;
		vaCreateImage(display, format, size.width(), size.height(), &image);
		Q_ASSERT(format->fourcc == image.format.fourcc);
	}
	~VaApiImage() {
		if (image.image_id != VA_INVALID_ID)
			vaDestroyImage(display, image.image_id);
	}
	VAImage image;
	bool ref = false;
	VADisplay display = VaApi::x11();
	VAImageFormat *format = nullptr;
};

typedef QLinkedList<VaApiImage*> ImagePool;
struct HwAccVaApiX11::Data {
	quint32 fourcc = 0;
	mp_imgfmt imgfmt = IMGFMT_NONE;
	ImagePool pool;
	ImagePool::iterator it;
	VAImageFormat format;
};

HwAccVaApiX11::HwAccVaApiX11(AVCodecID codec)
: HwAccVaApi(codec, VaApiX11), d(new Data) {
	d->it = d->pool.end();
}

HwAccVaApiX11::~HwAccVaApiX11() {
	for (auto image : d->pool) {
		image->format = nullptr;
		if (!image->ref)
			delete image;
	}
	delete d;
}

bool HwAccVaApiX11::check(AVCodecContext *avctx) {
	if (!HwAccVaApi::check(avctx))
		return false;
	retriveImageFormat();
	return HwAccVaApi::isOk();
}

void HwAccVaApiX11::retriveImageFormat() {
	VAImage image;
	auto destroy = [&image, this] () {
		if (image.image_id != VA_INVALID_ID) {
			vaDestroyImage(vaapi()->display, image.image_id);
			image.image_id = VA_INVALID_ID;
		}
	};
	auto findFormat =[&image, &destroy, this] () {
		int count = vaMaxNumImageFormats(vaapi()->display);
		QVector<VAImageFormat> formats;
		formats.resize(count);
		vaQueryImageFormats(vaapi()->display, formats.data(), &count);
		formats.resize(count);
		for (int i=0; i<formats.size(); ++i) {
			if (!isSuccess(vaCreateImage(vaapi()->display, &formats[i], size().width(), size().height(), &image))) {
				image.image_id = VA_INVALID_ID;
				continue;
			}
			if (!isSuccess(vaGetImage(vaapi()->display, static_cast<VaApiSurface*>(surface(0))->id, 0, 0, size().width(), size().height(), image.image_id))) {
				destroy();
				continue;
			}
			d->format = formats[i];
			switch (image.format.fourcc) {
			case VA_FOURCC_NV12:
				return IMGFMT_NV12;
			case VA_FOURCC_YV12:
			case VA_FOURCC('I', '4', '2', '0'):
				return IMGFMT_420P;
			case VA_FOURCC_YUY2:
				return IMGFMT_YUYV;
			case VA_FOURCC_UYVY:
				return IMGFMT_UYVY;
			default:
				status() = VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
				break;
			}
		}
		return IMGFMT_NONE;
	};
	d->imgfmt = findFormat();
	d->fourcc = image.format.fourcc;
	destroy();
}

VaApiImage *HwAccVaApiX11::newImage() {
	if (!d->pool.isEmpty()) {
		for (auto it = d->it; it != d->pool.end(); ++it) {
			if (!(*it)->ref)
				return *(d->it = it);
		}
		for (auto it = d->pool.begin(); it != d->it; ++it) {
			if (!(*it)->ref)
				return *(d->it = it);
		}
	}
	return *(d->it = d->pool.insert(d->pool.end(), new VaApiImage(&d->format, size())));
}

mp_image *HwAccVaApiX11::getImage(mp_image *mpi) {
	if (mpi->fmt.id != IMGFMT_VAAPI)
		return nullptr;
	const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
	vaSyncSurface(vaapi()->display, id);
	auto img = newImage();
	vaGetImage(vaapi()->display, id, 0, 0, size().width(), size().height(), img->image.image_id);
	void *data = nullptr;
	vaMapBuffer(vaapi()->display, img->image.buf, &data);
	img->ref = true;
	auto release = [](void *data) {
		auto img = static_cast<VaApiImage*>(data);
		vaUnmapBuffer(img->display, img->image.buf);
		img->ref = false;
		if (!img->format)
			delete img;
	};
	mpi = nullImage(d->imgfmt, img->image.width, img->image.height, img, release);
	uint offsets[3] = {img->image.offsets[0], img->image.offsets[1], img->image.offsets[2]};
	if (img->image.format.fourcc == VA_FOURCC_YV12)
		qSwap(offsets[1], offsets[2]);
	for (uint i=0; i<img->image.num_planes; ++i) {
		mpi->stride[i] = img->image.pitches[i];
		mpi->planes[i] = (uchar*)data + offsets[i];
	}
	return mpi;
}

#else
void initialize_vaapi() {}
void finalize_vaapi() {}
#endif

