#ifndef VIDEOFRAME_HPP
#define VIDEOFRAME_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

struct mp_image;

class VideoFrame {
public:
	VideoFrame(mp_image *mpi): d(new Data(mpi)) {}
	VideoFrame(): d(new Data) {}
	QImage toImage() const;
	const uchar *data(int i) const;
	const VideoFormat &format() const {return d->format;}
private:
	static quint32 UniqueId;
	struct Data : public QSharedData {
		Data() {}
		Data(mp_image *mpi);
		Data(const Data &other);
		~Data();
		Data &operator = (const Data &rhs) = delete;
		mp_image *mpi = nullptr;
		VideoFormat format;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFRAME_HPP
