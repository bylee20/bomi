#include "hwacc_vda.hpp"
extern "C" {
#include <video/mp_image.h>
}

#ifdef Q_OS_MAC

#include <OpenGL/CGLIOSurface.h>
#include <CoreVideo/CVOpenGLTextureCache.h>
#include <qpa/qplatformnativeinterface.h>
//#include <qpa/qplatformwindow.h>
extern "C" {
#include <libavcodec/vda.h>
}
#ifdef check
#undef check
#endif

struct HwAccVda::Data {
	vda_context context;
	AVPixelFormat pixfmt = AV_PIX_FMT_NONE;
	mp_imgfmt imgfmt = IMGFMT_NONE;
	bool ok = true;
};

HwAccVda::HwAccVda(AVCodecID codec)
: HwAcc(codec), d(new Data) {
	memset(&d->context, 0, sizeof(d->context));
}

HwAccVda::~HwAccVda() {
	freeContext();
	delete d;
}

mp_image *HwAccVda::getImage(mp_image *mpi) {
	auto buffer = (CVPixelBufferRef)mpi->planes[3];
	auto release = [] (void *arg) {
		CVPixelBufferRef buffer = (CVPixelBufferRef)arg;
//		IOSurfaceUnlock(CVPixelBufferGetIOSurface(buffer), 0, nullptr);
//		CVPixelBufferUnlockBaseAddress(buffer, 0);
		CVPixelBufferRelease(buffer);
	};
	CVPixelBufferRetain(buffer);
//	CVPixelBufferLockBaseAddress(buffer, 0);
//	auto surface = CVPixelBufferGetIOSurface(buffer);
//	IOSurfaceLock(surface, 0, nullptr);

	auto img = nullImage(IMGFMT_VDA, size().width(), size().height(), buffer, release);
	img->planes[3] = mpi->planes[3];
//	img->planes[0] = (uchar*)surface;
	return img;

	if (CVPixelBufferIsPlanar(buffer)) {
		for (int i=0; i<img->fmt.num_planes; ++i) {
			img->planes[i] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, i);
			img->stride[i] = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
		}
	} else {
		img->planes[0] = (uchar*)CVPixelBufferGetBaseAddress(buffer);
		img->stride[0] = CVPixelBufferGetBytesPerRow(buffer);
	}
	return img;
}

bool HwAccVda::isOk() const {
	return d->ok;
}

mp_image *HwAccVda::getSurface() {
	auto mpi = nullImage(IMGFMT_VDA, size().width(), size().height(), nullptr, nullptr);
	mpi->planes[0] = (uchar*)(void*)(uintptr_t)1;
	return mpi;
}

void *HwAccVda::context() const {
	return &d->context;
}

bool HwAccVda::check(AVCodecContext *avctx) {
	if (avctx->pix_fmt != AV_PIX_FMT_VDA_VLD || !isOk())
		return false;
	if (size().width() == avctx->width && size().height() == avctx->height)
		return true;
	if (!fillContext(avctx))
		return false;
	return true;
}

void HwAccVda::freeContext() {
	if (d->context.decoder) {
		ff_vda_destroy_decoder(&d->context);
		d->context.decoder = nullptr;
	}
}

bool HwAccVda::fillContext(AVCodecContext *avctx) {
	freeContext();

	d->ok = false;

	d->context.width = avctx->width;
	d->context.height = avctx->height;
	d->context.format = 'avc1';
	d->context.use_sync_decoding = 1;
	d->context.use_ref_buffer = 1;
	d->pixfmt = AV_PIX_FMT_YUYV422;//avcodec_default_get_format(avctx, avctx->codec->pix_fmts);
	switch (d->pixfmt) {
	case AV_PIX_FMT_UYVY422:
		d->context.cv_pix_fmt_type = kCVPixelFormatType_422YpCbCr8;
		d->imgfmt = IMGFMT_UYVY;
		break;
	case AV_PIX_FMT_YUYV422:
		d->context.cv_pix_fmt_type = kCVPixelFormatType_422YpCbCr8_yuvs;
		d->imgfmt = IMGFMT_YUYV;
		break;
	case AV_PIX_FMT_NV12:
		d->context.cv_pix_fmt_type = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
		d->imgfmt = IMGFMT_NV12;
		break;
	case AV_PIX_FMT_YUV420P:
		d->context.cv_pix_fmt_type = kCVPixelFormatType_420YpCbCr8Planar;
		d->imgfmt = IMGFMT_420P;
		break;
	default:
		qDebug() << "Not supported format:" << d->pixfmt;
	}

	if (kVDADecoderNoErr != ff_vda_create_decoder(&d->context, avctx->extradata, avctx->extradata_size))
		return false;
	size() = QSize(avctx->width, avctx->height);
	return (d->ok = true);
}

#endif
