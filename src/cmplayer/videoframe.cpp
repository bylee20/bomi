#include "videoframe.hpp"
#include <stdlib.h>
extern "C" {
#include <libmpcodecs/img_format.h>
}

uint32_t _fToImgFmt(VideoFormat::Type type) {
	switch (type) {
	case VideoFormat::YV12:
		return IMGFMT_YV12;
	case VideoFormat::I420:
		return IMGFMT_I420;
	case VideoFormat::YUY2:
		return IMGFMT_YUY2;
	default:
		return 0;
	}
}

QString _fToDescription(VideoFormat::Type type) {
	return QLatin1String(vo_format_name(_fToImgFmt(type)));
}


QImage VideoFrame::toImage() const {
	QImage img(format.stride, format.height, QImage::Format_RGB888);
	const int dy = format.stride;
	const int dy2 = dy << 1;
	const int dr = img.bytesPerLine();
	const int duv = format.stride/2;
	const uchar *y0 = y();
	const uchar *u0 = u();
	const uchar *v0 = v();
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



