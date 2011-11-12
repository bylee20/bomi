#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtCore/QList>
#include "avmisc.hpp"

class VideoFrame {
public:
	VideoFrame();
	VideoFrame(const VideoFormat &format);
	~VideoFrame();
	uchar *data() const {return m_data;}
	uchar *data(int idx) const {return m_data + m_offset[idx];}
	int dataPitch(int idx = 0) const {return m_format.planes[idx].data_pitch;}
	int dataLines(int idx = 0) const {return m_format.planes[idx].data_lines;}
	int framePitch(int idx = 0) const {return m_format.planes[idx].frame_pitch;}
	int frameLines(int idx = 0) const {return m_format.planes[idx].frame_lines;}
	int length() const {return m_length;}
	const VideoFormat &format() const {return m_format;}
	bool isEmpty() const {return m_length <= 0;}
	VideoFrame &operator= (const VideoFrame &rhs);
	VideoFrame(const VideoFrame &rhs);
private:
	void initLengthOffset() {
		m_length = 0;
		for (int i=0; i<m_format.plane_count; ++i) {
			m_offset[i] = m_length;
			m_length += m_format.planes[i].data_lines*m_format.planes[i].data_pitch;
		}
	}
	VideoFormat m_format;
	uchar *m_data;
	int m_length;
	int m_offset[VIDEO_FRAME_MAX_PLANE_COUNT];
};

#endif // VIDEOFRAME_HPP
