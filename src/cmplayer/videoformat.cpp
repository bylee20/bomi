#include "videoformat.hpp"
extern "C" {
#include <video/img_format.h>
}

VideoFormat::VideoFormat::Data::Data(const mp_image *mpi)
: size(mpi->w, mpi->h), drawSize(mpi->stride[0]/mpi->fmt.bytes[0], mpi->h)
, planes(mpi->fmt.num_planes), imgfmt(mpi->fmt.id) {
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
	default:
		pixfmt = AV_PIX_FMT_NONE;
		type = Unknown;
		break;
	}
	for (int i=0; i<3; ++i) {
		byteSize[i].rwidth() = mpi->stride[i];
		byteSize[i].rheight() = mpi->h >> mpi->fmt.ys[i];
		bpp += mpi->fmt.bpp[i] >> (mpi->fmt.xs[i] + mpi->fmt.ys[i]);
	}
}
