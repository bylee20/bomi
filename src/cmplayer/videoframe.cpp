#include "videoframe.hpp"
#include <stdlib.h>
extern "C" {
#include <video/img_format.h>
#include <video/mp_image.h>
}

extern "C" void *fast_memcpy(void * to, const void * from, size_t len);

void VideoFrame::setFormat(const VideoFormat &format) {
	d->format = format;
	for (int i=0; i<3; ++i)
		d->data[i].resize(format.byteWidth(i)*format.byteHeight(i));
}

bool VideoFrame::copy(GLuint *textures, GLenum fmt) {
	Q_ASSERT(fmt == GL_BGRA);
	Q_ASSERT(d->data[0].size() == d->format.width()*d->format.height()*4);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glGetTexImage(GL_TEXTURE_2D, 0, fmt, GL_UNSIGNED_BYTE, d->data[0].data());
	return false;
}

bool VideoFrame::copy(const mp_image *mpi) {
	bool ret = d->format.imgfmt() != mpi->imgfmt || d->format.byteWidth(0) != mpi->stride[0] || d->format.byteHeight(0) != mpi->height;
	if (ret)
		setFormat(VideoFormat::fromMpImage(mpi));
	for (int i=0; i<3; ++i)
		memcpy(d->data[i].data(), mpi->planes[i], d->data[i].size());
	return ret;
}

VideoFormat::Type imgfmtToVideoFormatType(unsigned int imgfmt) {
	switch (imgfmt) {
	case IMGFMT_YV12:
		return VideoFormat::YV12;
	case IMGFMT_I420:
		return VideoFormat::I420;
	case IMGFMT_YUY2:
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

VideoFormat VideoFormat::fromMpImage(const mp_image *mpi) {
	VideoFormat format;
	format.m_type = imgfmtToVideoFormatType(mpi->imgfmt);
	format.m_bpp = mpi->bpp;
	format.m_planes = mpi->num_planes;

	format.m_size = QSize(mpi->w, mpi->h);
	format.m_drawSize = QSize(mpi->stride[0], mpi->height);
	format.m_byteSize.fill(QSize(0, 0));
	for (int i=0; i<format.m_planes; ++i) {
		format.m_byteSize[i].rwidth() = mpi->stride[i];
		format.m_byteSize[i].rheight() = mpi->height;
	}

	format.m_imgfmt = mpi->imgfmt;
	switch (mpi->imgfmt) {
	case IMGFMT_YV12:
	case IMGFMT_I420:
		format.m_byteSize[1].rheight() >>= 1;
		format.m_byteSize[2].rheight() >>= 1;
		break;
	case IMGFMT_NV12:
	case IMGFMT_NV21:
		format.m_byteSize[1].rheight() >>= 1;
		break;
	case IMGFMT_YUY2:
	case IMGFMT_UYVY:
		format.m_drawSize.rwidth() >>= 1;
		break;
	case IMGFMT_RGBA:
	case IMGFMT_BGRA:
		format.m_drawSize.rwidth() >>= 2;
		break;
	default:
		break;
	}

	return format;
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
		return IMGFMT_YV12;
	case VideoFormat::I420:
		return IMGFMT_I420;
	case VideoFormat::YUY2:
		return IMGFMT_YUY2;
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


QImage VideoFrame::toImage() const {
	QImage img(d->format.size(), QImage::Format_RGB888);
	const int dy = d->format.drawWidth();
	const int dy2 = dy << 1;
	const int dr = img.bytesPerLine();
	const int duv = d->format.drawWidth()/2;
	const uchar *y0 = data(0);
	const uchar *u0 = data(1);
	const uchar *v0 = data(2);
	uchar *r1 = img.bits();

	auto setRgbFromYuv = [](uchar *&r, int y, int u, int v) -> void {
		y -= 16;	y *= 298;
		u -= 128;	v -= 128;
		*r++ = qBound(0, (y + 409*v + 128) >> 8, 255);
		*r++ = qBound(0, (y - 100*u - 208*v + 128) >> 8, 255);
		*r++ = qBound(0, (y + 516*u + 128) >> 8, 255);
	};

	for (int j = 0; j < img.height()/2; ++j) {
		const uchar *u = u0;	const uchar *v = v0;
		const uchar *y1 = y0;	const uchar *y2 = y1 + dy;
		uchar *r2 = r1 + dr;
		for (int i = 0; i < img.width()/2; ++i) {
			const int _u = *u++;
			const int _v = *v++;
			setRgbFromYuv(r1, *y1++, _u, _v);
			setRgbFromYuv(r1, *y1++, _u, _v);
			setRgbFromYuv(r2, *y2++, _u, _v);
			setRgbFromYuv(r2, *y2++, _u, _v);
		}
		r1 += dr;		y0 += dy2;
		u0 += duv;		v0 += duv;
	}
	return img;
}


//VideoFrame::VideoFrame(): m_data(0), m_length(0) {}

//VideoFrame::VideoFrame(const VideoFormat &format)
//: m_format(format), m_data(0), m_length(0) {
//	for (int i=0; i<m_format.plane_count; ++i) {
//		m_offset[i] = m_length;
//		m_length += m_format.planes[i].data_lines*m_format.planes[i].data_pitch;
//	}
//	if (m_length > 0)
//		m_data = new uchar[m_length];
//}

//VideoFrame::VideoFrame(const VideoFrame &rhs)
//: m_format(rhs.m_format), m_data(0), m_length(rhs.m_length) {
//	if (m_length > 0) {
//		m_data = new uchar[m_length];
//		memcpy(m_data, rhs.m_data, m_length);
//	}
//	m_offset[0] = rhs.m_offset[0];
//	m_offset[1] = rhs.m_offset[1];
//	m_offset[2] = rhs.m_offset[2];
//}

//VideoFrame::~VideoFrame() {
//	delete [] m_data;
//}

//VideoFrame &VideoFrame::operator= (const VideoFrame &rhs) {
//	if (this != &rhs) {
//		m_format = rhs.m_format;
//		if (m_length != rhs.m_length) {
//			delete [] m_data;
//			m_length = rhs.m_length;
//			m_data = (m_length > 0) ? new uchar[m_length] : 0;
//		}
//		if (m_data)
//			memcpy(m_data, rhs.m_data, m_length);
//		m_offset[0] = rhs.m_offset[0];
//		m_offset[1] = rhs.m_offset[1];
//		m_offset[2] = rhs.m_offset[2];
//	}
//	return *this;
//}



