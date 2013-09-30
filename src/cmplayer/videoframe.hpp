#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

class VideoFrame {
public:
	enum Field {Picture = 0, Top = 1, Bottom = 2, Interlaced = Top | Bottom, Additional = 4, Flipped = 8};
	VideoFrame(bool free, mp_image *mpi, const VideoFormat &format, double pts, int field = Picture): d(new Data(free, mpi, format, pts, field)) {}
	VideoFrame(bool free, mp_image *mpi, double pts, int field = Picture): d(new Data(free, mpi, VideoFormat(mpi), pts, field)) {}
	VideoFrame(bool free, mp_image *mpi, int field = Picture): d(new Data(free, mpi, VideoFormat(mpi), mpi->pts, field)) {}
	VideoFrame(bool free, mp_image *mpi, const VideoFormat &format, int field = Picture): d(new Data(free, mpi, format, mpi->pts, field)) {}
	VideoFrame(const QImage &image): d(new Data(image)) {}
	VideoFrame(): d(new Data) {}
	VideoFrame(VideoFrame &&other): d(nullptr) { other.d.swap(d); }
	VideoFrame(const VideoFrame &other): d(other.d) {}
	VideoFrame &operator = (const VideoFrame &other) { d = other.d; return *this; }
	VideoFrame &operator = (VideoFrame &&other) { other.d.swap(d); return *this; }
	QImage toImage() const;
	bool isFlipped() const { return d->field & Flipped; }
	bool hasImage() const {return !d->image.isNull();}
	const QImage &image() const {return d->image;}
	const uchar *data(int i) const {return d->data[i];}
	const VideoFormat &format() const {return d->format;}
	int width() const {return d->format.width();}
	int height() const {return d->format.height();}
	double pts() const {return d->pts;}
//	void setField(int field) { d->field = field; }
	int field() const { return d->field; }
	void setPts(double pts) { d->pts = pts; }
	mp_image *mpi() const { return d->mpi; }
	void swap(VideoFrame &other) {d.swap(other.d);}
	double nextPts(double prevPts, int split = 2) const {
		if (prevPts == MP_NOPTS_VALUE)
			return d->pts;
		const auto diff = (d->pts - prevPts);
		return (0.0 < diff && diff < 0.5) ? (d->pts + diff/(double)(split)) : d->pts;
	}
	void allocate(const VideoFormat &format);
	void doDeepCopy(const VideoFrame &frame);
	bool isAdditional() const { return d->field & Additional; }
	bool isInterlaced() const { return d->field & Interlaced; }
	bool isTopField() const { return d->field & Top; }
	bool isBottomField() const { return d->field & Bottom; }
private:
	struct Data : public QSharedData {
		Data() {}
		Data(bool free, mp_image *mpi, const VideoFormat &format, double pts, int field);
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
		QByteArray buffer;
	};
	QSharedDataPointer<Data> d;
};

Q_DECLARE_TYPEINFO(VideoFrame, Q_MOVABLE_TYPE);

#endif // VIDEOFRAME_HPP
