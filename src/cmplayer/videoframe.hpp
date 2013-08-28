#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

class VideoFrame {
public:
	enum Field {Picture = 0, Top = 1, Bottom = 2, Last = 4, Interlaced = Top | Bottom};
	VideoFrame(mp_image *mpi, const VideoFormat &format): d(new Data(mpi, format)) {}
	VideoFrame(const QImage &image): d(new Data(image)) {}
	VideoFrame(): d(new Data) {}
	QImage toImage() const;
	bool hasImage() const {return !d->image.isNull();}
	const QImage &image() const {return d->image;}
	const uchar *data(int i) const {return d->data[i];}
	const VideoFormat &format() const {return d->format;}
	int width() const {return d->format.width();}
	int height() const {return d->format.height();}
	double pts() const {return d->pts;}
	void setField(int field) { d->field = field; }
	int field() const { return d->field; }
	void setPts(double pts) { d->pts = pts; }
	const mp_image *mpi() const { return d->mpi; }
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
		double pts = MP_NOPTS_VALUE;
		int field = Picture;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFRAME_HPP
