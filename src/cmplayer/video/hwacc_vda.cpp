#include "hwacc_vda.hpp"

#ifdef Q_OS_MAC
#include "hwacc_helper.hpp"
#include "openglcompat.hpp"
#include "misc/log.hpp"
#include <OpenGL/CGLIOSurface.h>
#include <OpenGL/OpenGL.h>
#include <CoreVideo/CVPixelBuffer.h>
extern "C" {
#include <video/mp_image.h>
#include <libavcodec/vda.h>
}
#ifdef check
#undef check
#endif

DECLARE_LOG_CONTEXT(VDA)

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

auto HwAccVda::getImage(mp_image *mpi) -> mp_image*
{
    auto buffer = (CVPixelBufferRef)mpi->planes[3];
    auto release = [] (void *arg) {
        CVPixelBufferRef buffer = (CVPixelBufferRef)arg;
        CVPixelBufferRelease(buffer);
    };
    CVPixelBufferRetain(buffer);
    auto img = null_mp_image(IMGFMT_VDA, size().width(), size().height(), buffer, release);
    mp_image_copy_attributes(img, mpi);
    img->planes[3] = mpi->planes[3];
    return img;
}

auto HwAccVda::isOk() const -> bool
{
    return d->ok;
}

auto HwAccVda::getSurface() -> mp_image*
{
    auto mpi = null_mp_image(IMGFMT_VDA, size().width(), size().height(), nullptr, nullptr);
    mpi->planes[0] = (uchar*)(void*)(uintptr_t)1;
    return mpi;
}

auto HwAccVda::context() const -> void*
{
    return &d->context;
}

auto HwAccVda::freeContext() -> void
{
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
    if (!_Contains(cvpixfmts, fmt)) {
        qDebug() << "vda: not supported format!!";
        return;
    }
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

auto HwAccVda::fillContext(AVCodecContext *avctx) -> bool
{
    freeContext();
    d->ok = false;
    d->context.width = avctx->width;
    d->context.height = avctx->height;
    d->context.format = 'avc1';
    d->context.use_sync_decoding = 1;
    d->context.use_ref_buffer = 1;
    if (kVDADecoderNoErr != _ff_vda_create_decoder(&d->context, avctx->extradata, avctx->extradata_size))
        return false;
    return (d->ok = true);
}

/*****************************************************************************/

static inline bool directRendering(mp_imgfmt type) {
    return OGL::hasExtension(OGL::AppleYCbCr422) && _IsOneOf(type, IMGFMT_YUYV, IMGFMT_UYVY);
}

VdaMixer::VdaMixer(const QList<OpenGLTexture2D> &textures, const VideoFormat &format)
: m_textures(textures) {
    Q_ASSERT(format.imgfmt() == IMGFMT_VDA);
    m_direct = ::directRendering(format.type());
}

auto VdaMixer::upload(const VideoFrame &frame, bool /*deint*/) -> bool
{
    Q_ASSERT(frame.format().imgfmt() == IMGFMT_VDA);
    CGLError error = kCGLNoError;
    for (auto &texture : m_textures) {
        const auto cgl = CGLGetCurrentContext();
        const auto surface = CVPixelBufferGetIOSurface((CVPixelBufferRef)frame.data(3));
        texture.bind();
        const auto w = IOSurfaceGetWidthOfPlane(surface, texture.plane());
        const auto h = IOSurfaceGetHeightOfPlane(surface, texture.plane());
        if (_Change(error, CGLTexImageIOSurface2D(cgl, texture.target(), texture.format(), w, h, texture.transfer().format, texture.transfer().type, surface, texture.plane()))) {
            _Error("CGLError: %%(0x%%)", CGLErrorString(error), _N(error, 16));
            return false;
        }
    }
    return true;
}

auto VdaMixer::adjust(VideoFormatData *data, const mp_image *mpi) -> void
{
    Q_ASSERT(data->imgfmt == IMGFMT_VDA);
    auto buffer = (CVPixelBufferRef)mpi->planes[3];
    switch (CVPixelBufferGetPixelFormatType(buffer)) {
    case kCVPixelFormatType_422YpCbCr8:
        data->type = IMGFMT_UYVY;
        break;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        data->type = IMGFMT_YUYV;
        break;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
        data->type = IMGFMT_NV12;
        break;
    case kCVPixelFormatType_420YpCbCr8Planar:
        data->type = IMGFMT_420P;
        break;
    default:
        _Error("Not supported format.");
        data->type = IMGFMT_NONE;
    }
    auto desc = mp_imgfmt_get_desc(data->type);
    if (CVPixelBufferIsPlanar(buffer)) {
        data->planes = CVPixelBufferGetPlaneCount(buffer);
        Q_ASSERT(data->planes == desc.num_planes);
        for (int i=0; i<data->planes; ++i) {
            data->alignedByteSize[i].rwidth() = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
            data->alignedByteSize[i].rheight() = CVPixelBufferGetHeightOfPlane(buffer, i);
            data->bpp += desc.bpp[i] >> (desc.xs[i] + desc.ys[i]);
        }
    } else {
        data->planes = 1;
        data->alignedByteSize[0].rwidth() = CVPixelBufferGetBytesPerRow(buffer);
        data->alignedByteSize[0].rheight() = CVPixelBufferGetHeight(buffer);
        data->bpp = desc.bpp[0];
    }
    data->alignedSize = data->alignedByteSize[0];
    data->alignedSize.rwidth() /= desc.bytes[0];
    if (::directRendering(data->type))
        data->colorspace = MP_CSP_RGB;
}

#endif
