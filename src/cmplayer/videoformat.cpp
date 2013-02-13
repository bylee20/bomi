#include "videoformat.hpp"
extern "C" {
#include "video/mp_image.h"
#include "video/img_format.h"
}

VideoFormat::VideoFormat(const mp_image *mpi) {
	m_size = QSize(mpi->w, mpi->h);
	m_byteSize.fill(QSize(0, 0));
	m_type = imgfmtToVideoFormatType(mpi->fmt.id);
	m_planes = mpi->fmt.num_planes;
	m_drawSize = QSize(mpi->stride[0]/mpi->fmt.bytes[0], mpi->h);
	m_bpp = 0;
	for (int i=0; i<m_planes; ++i) {
		m_byteSize[i].rwidth() = mpi->stride[i];
		m_byteSize[i].rheight() = mpi->h >> mpi->fmt.ys[i];
		m_bpp += mpi->fmt.bpp[i] >> (mpi->fmt.xs[i] + mpi->fmt.ys[i]);
	}
}

VideoFormat::Type imgfmtToVideoFormatType(unsigned int imgfmt) {
	switch (imgfmt) {
	case IMGFMT_420P:
		return VideoFormat::I420;
	case IMGFMT_YUYV:
		return VideoFormat::YUY2;
	case IMGFMT_UYVY:
		return VideoFormat::UYVY;
	case IMGFMT_NV12:
		return VideoFormat::NV12;
	case IMGFMT_NV21:
		return VideoFormat::NV21;
	case IMGFMT_RGBA:
		return VideoFormat::RGBA;
	case IMGFMT_BGRA:
		return VideoFormat::BGRA;
	default:
		return VideoFormat::Unknown;
	}
}


//VideoFormat VideoFormat::fromImgFmt(uint32_t imgfmt, int width, int height) {
//	switch (imgfmt) {
//	case IMGFMT_YV12:
//		return fromType(YV12, width, height);
//	case IMGFMT_I420:
//		return fromType(I420, width, height);
//	case IMGFMT_YUY2:
//		return fromType(YUY2, width, height);
//	case IMGFMT_UYVY:
//		return fromType(UYVY, width, height);
//	case IMGFMT_NV12:
//		return fromType(NV12, width, height);
//	case IMGFMT_NV21:
//		return fromType(NV21, width, height);
//	case IMGFMT_RGBA:
//		return fromType(RGBA, width, height);
//	default:
//		return VideoFormat();
//	}
//}

uint32_t videoFormatTypeToImgfmt(VideoFormat::Type type) {
	switch (type) {
	case VideoFormat::YV12:
		return IMGFMT_420P;
	case VideoFormat::YUY2:
		return IMGFMT_YUYV;
	case VideoFormat::UYVY:
		return IMGFMT_UYVY;
	case VideoFormat::NV12:
		return IMGFMT_NV12;
	case VideoFormat::NV21:
		return IMGFMT_NV21;
	case VideoFormat::BGRA:
		return IMGFMT_BGRA;
	case VideoFormat::RGBA:
		return IMGFMT_RGBA;
	default:
		return 0;
	}
}

QString cc4ToDescription(VideoFormat::Type type) {
	return _L(vo_format_name(videoFormatTypeToImgfmt(type)));
}

