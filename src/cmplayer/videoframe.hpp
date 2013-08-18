#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

class VideoFrame {
public:
	VideoFrame(mp_image *mpi, const VideoFormat &format): d(new Data(mpi, format)) {}
	VideoFrame(const QImage &image): d(new Data(image)) {}
	VideoFrame(): d(new Data) {}
	QImage toImage() const;
	bool hasImage() const {return !d->image.isNull();}
	const QImage &image() const {return d->image;}
	const uchar *data(int i) const {return d->data[i];}
	const VideoFormat &format() const {return d->format;}
	bool isNative() const {return d->native;}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(mp_image *mpi, const VideoFormat &format);
		Data(const QImage &image);
		Data(const Data &other);
		~Data();
		Data &operator = (const Data &rhs) = delete;
		mp_image *mpi = nullptr;
		QImage image;
		uchar *data[4] = {0, 0, 0, 0};
		VideoFormat format;
		bool native = false;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFRAME_HPP
