#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"
extern "C" {
#include <mp_image.h>
}

struct mp_image;

class VideoFrame {
	VideoFrame(const VideoFrame&);
	VideoFrame &operator=(const VideoFrame&);
public:
	VideoFrame(): d(new Data) {}
	bool copy(mp_image *mpi) {
		bool ret = false;
		if (d->format.stride() != mpi->stride[0]) {
			d->format = VideoFormat::fromMpImage(mpi);
//			qDebug() << d->format.width() << d->format.storedWidth() << d->format.stride();
			ret = true;
		}
		d->copy(0, mpi);d->copy(1, mpi);d->copy(2, mpi);d->copy(3, mpi);
		return ret;
	}
	void setFormat(const VideoFormat &format) {d->format = format;}
	const VideoFormat &format() const {return d->format;}
	VideoFormat &rformat() {return d->format;}
	QImage toImage() const;
	int stride(int i) const {return d->stride[i];}
	uchar *data(int i) {return reinterpret_cast<uchar*>(d->data[i].data());}
	const uchar *data(int i) const {return reinterpret_cast<const uchar*>(d->data[i].data());}
	inline void swap(VideoFrame &other) {d.swap(other.d);}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const Data &other): QSharedData(other) {
			data[0] = other.data[0];
			data[1] = other.data[1];
			data[2] = other.data[2];
			data[3] = other.data[3];
			stride[0] = other.stride[0];
			stride[1] = other.stride[1];
			stride[2] = other.stride[2];
			stride[3] = other.stride[3];
		}
		void copy(int idx, mp_image *mpi) {
			stride[idx] = mpi->stride[idx];
			data[idx].resize(stride[idx]*mpi->height);
			fast_memcpy(data[idx].data(), mpi->planes[idx], data[idx].size());
		}
		VideoFormat format;
		QByteArray data[4];
		int stride[4];
	};

	QSharedDataPointer<Data> d;
//	struct Plane {
//		void copy(const uchar *data, int stride, int height) {
//			this->data.resize(stride*height);
//			this->stride = stride;
//			fast_memcpy(this->data.data(), data, this->data.size());
//		}
//		QByteArray data; int stride;
//	};
//	QVector<Plane> m_planes = {4, Plane()};
};

//class VideoFrame {
//public:
//	VideoFrame();
//	VideoFrame(const VideoFormat &format);
//	~VideoFrame();
//	uchar *data() const {return m_data;}
//	uchar *data(int idx) const {return m_data + m_offset[idx];}
//	int dataPitch(int idx = 0) const {return m_format.planes[idx].data_pitch;}
//	int dataLines(int idx = 0) const {return m_format.planes[idx].data_lines;}
//	int framePitch(int idx = 0) const {return m_format.planes[idx].frame_pitch;}
//	int frameLines(int idx = 0) const {return m_format.planes[idx].frame_lines;}
//	int length() const {return m_length;}
//	const VideoFormat &format() const {return m_format;}
//	bool isEmpty() const {return m_length <= 0;}
//	VideoFrame &operator= (const VideoFrame &rhs);
//	VideoFrame(const VideoFrame &rhs);
//private:
//	void initLengthOffset() {
//		m_length = 0;
//		for (int i=0; i<m_format.plane_count; ++i) {
//			m_offset[i] = m_length;
//			m_length += m_format.planes[i].data_lines*m_format.planes[i].data_pitch;
//		}
//	}
//	VideoFormat m_format;
//	uchar *m_data;
//	int m_length;
//	int m_offset[VIDEO_FRAME_MAX_PLANE_COUNT];
//};

#endif // VIDEOFRAME_HPP
