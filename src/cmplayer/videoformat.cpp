#include "videoformat.hpp"
#ifdef Q_OS_MAC
#include <VideoDecodeAcceleration/VDADecoder.h>
#endif
extern "C" {
#include <video/img_format.h>
#include <libavutil/common.h>
}

VideoFormat::VideoFormat::Data::Data(const mp_image *mpi, int dest_w, int dest_h)
: size(mpi->w, mpi->h), outputSize(dest_w, dest_h)
, planes(mpi->fmt.num_planes), type(mpi->imgfmt), imgfmt(mpi->imgfmt)
, colorspace(mpi->colorspace), range(mpi->levels) {
	if ((native = IMGFMT_IS_HWACCEL(imgfmt))) {
#ifdef Q_OS_LINUX
		if (imgfmt == IMGFMT_VAAPI) {
			type = IMGFMT_BGRA;
			planes = 1;
			const int stride = FFALIGN((mpi->w * 32 + 7) / 8, 16);
			alignedSize = QSize(stride/4, mpi->h);
			alignedByteSize[0] = QSize(stride, mpi->h);
			bpp = 32;
		}
#endif
#ifdef Q_OS_MAC
		if (imgfmt == IMGFMT_VDA) {
			auto buffer = (CVPixelBufferRef)mpi->planes[3];
			switch (CVPixelBufferGetPixelFormatType(buffer)) {
			case kCVPixelFormatType_422YpCbCr8:
				type = IMGFMT_UYVY;
				break;
			case kCVPixelFormatType_422YpCbCr8_yuvs:
				type = IMGFMT_YUYV;
				break;
			case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
				type = IMGFMT_NV12;
				break;
			case kCVPixelFormatType_420YpCbCr8Planar:
				type = IMGFMT_420P;
				break;
			default:
				qDebug() << "Not supported format!";
				type = IMGFMT_NONE;
			}
			auto desc = mp_imgfmt_get_desc(type);
			if (CVPixelBufferIsPlanar(buffer)) {
				planes = CVPixelBufferGetPlaneCount(buffer);
				Q_ASSERT(planes == desc.num_planes);
				for (int i=0; i<planes; ++i) {
					alignedByteSize[i].rwidth() = CVPixelBufferGetBytesPerRowOfPlane(buffer, i);
					alignedByteSize[i].rheight() = CVPixelBufferGetHeightOfPlane(buffer, i);
					bpp += desc.bpp[i] >> (desc.xs[i] + desc.ys[i]);
				}
			} else {
				planes = 1;
				alignedByteSize[0].rwidth() = CVPixelBufferGetBytesPerRow(buffer);
				alignedByteSize[0].rheight() = CVPixelBufferGetHeight(buffer);
				bpp = desc.bpp[0];
			}
			alignedSize = alignedByteSize[0];
			alignedSize.rwidth() /= desc.bytes[0];
		}
#endif
	} else {
		alignedSize = QSize(mpi->stride[0]/mpi->fmt.bytes[0], mpi->h);
		for (int i=0; i<3; ++i) {
			alignedByteSize[i].rwidth() = mpi->stride[i];
			alignedByteSize[i].rheight() = mpi->h >> mpi->fmt.ys[i];
			bpp += mpi->fmt.bpp[i] >> (mpi->fmt.xs[i] + mpi->fmt.ys[i]);
		}
	}
}

VideoFormat::VideoFormat::Data::Data(const QImage &image) {
	outputSize = alignedSize = size = image.size();
	alignedByteSize[0] = QSize(size.width()*4, size.height());
	planes = 1;
	bpp = 32;
	type = imgfmt = IMGFMT_BGRA;
	colorspace = MP_CSP_RGB;
	range = MP_CSP_LEVELS_PC;
}

QString VideoFormat::name() const {
	return QString::fromLatin1(mp_imgfmt_to_name(d->type));
}
