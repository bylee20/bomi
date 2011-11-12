#include "videoframe.hpp"
#include <stdlib.h>

VideoFrame::VideoFrame(): m_data(0), m_length(0) {}

VideoFrame::VideoFrame(const VideoFormat &format)
: m_format(format), m_data(0), m_length(0) {
	for (int i=0; i<m_format.plane_count; ++i) {
		m_offset[i] = m_length;
		m_length += m_format.planes[i].data_lines*m_format.planes[i].data_pitch;
	}
	if (m_length > 0)
		m_data = new uchar[m_length];
}

VideoFrame::VideoFrame(const VideoFrame &rhs)
: m_format(rhs.m_format), m_data(0), m_length(rhs.m_length) {
	if (m_length > 0) {
		m_data = new uchar[m_length];
		memcpy(m_data, rhs.m_data, m_length);
	}
	m_offset[0] = rhs.m_offset[0];
	m_offset[1] = rhs.m_offset[1];
	m_offset[2] = rhs.m_offset[2];
}

VideoFrame::~VideoFrame() {
	delete [] m_data;
}

VideoFrame &VideoFrame::operator= (const VideoFrame &rhs) {
	if (this != &rhs) {
		m_format = rhs.m_format;
		if (m_length != rhs.m_length) {
			delete [] m_data;
			m_length = rhs.m_length;
			m_data = (m_length > 0) ? new uchar[m_length] : 0;
		}
		if (m_data)
			memcpy(m_data, rhs.m_data, m_length);
		m_offset[0] = rhs.m_offset[0];
		m_offset[1] = rhs.m_offset[1];
		m_offset[2] = rhs.m_offset[2];
	}
	return *this;
}



