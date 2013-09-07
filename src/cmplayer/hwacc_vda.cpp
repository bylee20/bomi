#include "hwacc_vda.hpp"
extern "C" {
#include <video/mp_image.h>
}

#ifdef Q_OS_MAC

#include <OpenGL/CGLIOSurface.h>
#include <CoreVideo/CVOpenGLTextureCache.h>
#include <qpa/qplatformnativeinterface.h>
extern "C" {
#include <libavcodec/vda.h>
#define class ____class
#define new ____new
#define Picture ____Picture
#include <libavcodec/h264.h>
#undef Picture
#undef new
#undef class
}
#ifdef check
#undef check
#endif

struct HwAccVda::Data {
	vda_context context;
	bool ok = true;
};

const std::array<OSType, 4> cvpixfmts{{
	kCVPixelFormatType_420YpCbCr8Planar,
	kCVPixelFormatType_422YpCbCr8,
	kCVPixelFormatType_422YpCbCr8_yuvs,
	kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
}};

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
		CVPixelBufferRelease(buffer);
	};
	CVPixelBufferRetain(buffer);
	auto img = nullMpImage(IMGFMT_VDA, size().width(), size().height(), buffer, release);
	mp_image_copy_attributes(img, mpi);
	img->planes[3] = mpi->planes[3];
	return img;
}

bool HwAccVda::isOk() const {
	return d->ok;
}

mp_image *HwAccVda::getSurface() {
	auto mpi = nullMpImage(IMGFMT_VDA, size().width(), size().height(), nullptr, nullptr);
	mpi->planes[0] = (uchar*)(void*)(uintptr_t)1;
	return mpi;
}

void *HwAccVda::context() const {
	return &d->context;
}

void HwAccVda::freeContext() {
	if (d->context.decoder) {
		ff_vda_destroy_decoder(&d->context);
		d->context.decoder = nullptr;
	}
}

static CFArrayRef vda_create_pixel_format_array() {
	auto array = CFArrayCreateMutable(kCFAllocatorDefault, 4, &kCFTypeArrayCallBacks);
	for (auto type : cvpixfmts) {
		auto format = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &type);
		CFArrayAppendValue(array, format);
		CFRelease(format);
	}
	return array;
}

/* Decoder callback that adds the vda frame to the queue in display order. */
static void vda_decoder_callback (void *vda_hw_ctx, CFDictionaryRef /*user_info*/, OSStatus /*status*/, uint32_t /*infoFlags*/, CVImageBufferRef image_buffer) {
	vda_context *vda_ctx = (vda_context*)vda_hw_ctx;

	if (!image_buffer)
		return;
	const auto fmt = CVPixelBufferGetPixelFormatType(image_buffer);
	if (!_Contains(cvpixfmts, fmt))
		return;
	vda_ctx->cv_pix_fmt_type = fmt;
	vda_ctx->cv_buffer = CVPixelBufferRetain(image_buffer);
}

static int _ff_vda_create_decoder(struct vda_context *vda_ctx, uint8_t *extradata, int extradata_size) {
	OSStatus status;
	CFNumberRef height;
	CFNumberRef width;
	CFNumberRef format;
	CFDataRef avc_data;
	CFMutableDictionaryRef config_info;
	CFMutableDictionaryRef buffer_attributes;
	CFMutableDictionaryRef io_surface_properties;
	CFNumberRef cv_pix_fmt;

	vda_ctx->priv_bitstream = NULL;
	vda_ctx->priv_allocated_size = 0;

	/* Each VCL NAL in the bitstream sent to the decoder
	 * is preceded by a 4 bytes length header.
	 * Change the avcC atom header if needed, to signal headers of 4 bytes. */
	if (extradata_size >= 4 && (extradata[4] & 0x03) != 0x03) {
		uint8_t *rw_extradata;

		if (!(rw_extradata = (uchar*)av_malloc(extradata_size)))
			return AVERROR(ENOMEM);

		memcpy(rw_extradata, extradata, extradata_size);

		rw_extradata[4] |= 0x03;

		avc_data = CFDataCreate(kCFAllocatorDefault, rw_extradata, extradata_size);

		av_freep(&rw_extradata);
	} else {
		avc_data = CFDataCreate(kCFAllocatorDefault, extradata, extradata_size);
	}

	config_info = CFDictionaryCreateMutable(kCFAllocatorDefault,
											4,
											&kCFTypeDictionaryKeyCallBacks,
											&kCFTypeDictionaryValueCallBacks);

	height   = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vda_ctx->height);
	width    = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vda_ctx->width);
	format   = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vda_ctx->format);

	CFDictionarySetValue(config_info, kVDADecoderConfiguration_Height, height);
	CFDictionarySetValue(config_info, kVDADecoderConfiguration_Width, width);
	CFDictionarySetValue(config_info, kVDADecoderConfiguration_SourceFormat, format);
	CFDictionarySetValue(config_info, kVDADecoderConfiguration_avcCData, avc_data);

	buffer_attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
												  2,
												  &kCFTypeDictionaryKeyCallBacks,
												  &kCFTypeDictionaryValueCallBacks);
	io_surface_properties = CFDictionaryCreateMutable(kCFAllocatorDefault,
													  0,
													  &kCFTypeDictionaryKeyCallBacks,
													  &kCFTypeDictionaryValueCallBacks);
	cv_pix_fmt  = CFNumberCreate(kCFAllocatorDefault,
								 kCFNumberSInt32Type,
								 &vda_ctx->cv_pix_fmt_type);
	auto formats = vda_create_pixel_format_array();
	CFDictionarySetValue(buffer_attributes,
						 kCVPixelBufferPixelFormatTypeKey,
						 formats);
	CFDictionarySetValue(buffer_attributes,
						 kCVPixelBufferIOSurfacePropertiesKey,
						 io_surface_properties);

	status = VDADecoderCreate(config_info,
							  buffer_attributes,
							  (VDADecoderOutputCallback*)vda_decoder_callback,
							  vda_ctx,
							  &vda_ctx->decoder);

	CFRelease(formats);
	CFRelease(height);
	CFRelease(width);
	CFRelease(format);
	CFRelease(avc_data);
	CFRelease(config_info);
	CFRelease(io_surface_properties);
	CFRelease(cv_pix_fmt);
	CFRelease(buffer_attributes);

	return status;
}

bool HwAccVda::fillContext(AVCodecContext *avctx) {
	freeContext();

	d->ok = false;

	const auto h264 = static_cast<H264Context*>(avctx->priv_data);
	if (h264 && h264->sps.pic_struct_present_flag && h264->sei_pic_struct != SEI_PIC_STRUCT_FRAME)
		return false;
	if (h264 && FIELD_OR_MBAFF_PICTURE(h264))
		return false;

	d->context.width = avctx->width;
	d->context.height = avctx->height;
	d->context.format = 'avc1';
	d->context.use_sync_decoding = 1;
	d->context.use_ref_buffer = 1;

	if (kVDADecoderNoErr != _ff_vda_create_decoder(&d->context, avctx->extradata, avctx->extradata_size))
		return false;
	return (d->ok = true);
}

#endif
