#include "videoformat.hpp"
extern "C" {
#include <video/img_format.h>
#include <libavutil/common.h>
}

VideoFormat::VideoFormat::Data::Data(const mp_image *mpi, int dest_w, int dest_h)
: size(mpi->w, mpi->h), outputSize(dest_w, dest_h)
, planes(mpi->fmt.num_planes), imgfmt(mpi->fmt.id), glFormat(GL_UNSIGNED_BYTE) {
	switch (imgfmt) {
	case IMGFMT_420P:
		pixfmt = AV_PIX_FMT_YUV420P;
		type = I420;
		break;
	case IMGFMT_YUYV:
		pixfmt = AV_PIX_FMT_YUYV422;
		type = YUY2;
		break;
	case IMGFMT_UYVY:
		pixfmt = AV_PIX_FMT_UYVY422;
		type = UYVY;
		break;
	case IMGFMT_NV12:
		pixfmt = AV_PIX_FMT_NV12;
		type = NV12;
		break;
	case IMGFMT_NV21:
		pixfmt = AV_PIX_FMT_NV21;
		type = NV21;
		break;
	case IMGFMT_RGBA:
		pixfmt = AV_PIX_FMT_RGBA;
		type = RGBA;
		break;
	case IMGFMT_BGRA:
		pixfmt = AV_PIX_FMT_BGRA;
		type = BGRA;
		break;
	case IMGFMT_VAAPI:
		pixfmt = AV_PIX_FMT_VAAPI_VLD;
		type = VAGL;
		break;
	default:
		pixfmt = AV_PIX_FMT_NONE;
		type = Unknown;
		break;
	}
	if (type != VAGL) {
		alignedSize = QSize(mpi->stride[0]/mpi->fmt.bytes[0], mpi->h);
		for (int i=0; i<3; ++i) {
			alignedByteSize[i].rwidth() = mpi->stride[i];
			alignedByteSize[i].rheight() = mpi->h >> mpi->fmt.ys[i];
			bpp += mpi->fmt.bpp[i] >> (mpi->fmt.xs[i] + mpi->fmt.ys[i]);
		}
	} else {
		planes = 1;
		const int stride = FFALIGN((mpi->w * 32 + 7) / 8, 16);
		alignedSize = QSize(stride/4, mpi->h);
		alignedByteSize[0] = QSize(stride, mpi->h);
		bpp = 32;
	}
}

VideoFormat::VideoFormat::Data::Data(const QImage &image) {
	Q_ASSERT(image.format() == QImage::Format_ARGB32 || image.format() == QImage::Format_ARGB32_Premultiplied);
	alignedSize = size = image.size();
	alignedByteSize[0] = QSize(size.width()*4, size.height());
	planes = 1;
	bpp = 32;
	imgfmt = IMGFMT_BGRA;
	pixfmt = AV_PIX_FMT_BGRA;
	type = BGRA;
	glFormat = GL_UNSIGNED_INT_8_8_8_8_REV;
}
