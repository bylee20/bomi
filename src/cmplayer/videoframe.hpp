#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"
extern "C" {
#include <mp_image.h>
}

struct mp_image;

class VideoFrame {
	VideoFrame(const VideoFrame&) = delete;
	VideoFrame &operator=(const VideoFrame&) = delete;
public:
	VideoFrame(): d(new Data) {}
	bool copy(mp_image *mpi);
	const VideoFormat &format() {return d->format;}
	void setFormat(const VideoFormat &format);
	QImage toImage() const;
	uchar *data(int i) {return reinterpret_cast<uchar*>(d->data[i].data());}
	const uchar *data(int i) const {return reinterpret_cast<const uchar*>(d->data[i].data());}
	inline void swap(VideoFrame &other) {d.swap(other.d);}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const Data &other): QSharedData(other) {
			format = other.format;		data[0] = other.data[0];
			data[1] = other.data[1];	data[2] = other.data[2];
		}
		Data &operator = (const Data &rhs) = delete;
		VideoFormat format;
		QByteArray data[3];
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFRAME_HPP
