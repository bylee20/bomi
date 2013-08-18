#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include "stdafx.hpp"
extern "C" {
#include <video/mp_image.h>
#include <libavutil/pixfmt.h>
}
//struct mp_image;

constexpr static inline quint32 cc4(char a, char b, char c, char d) {
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
	return (((quint32)d)|(((quint32)c)<<8)|(((quint32)b)<<16)|(((quint32)a)<<24));
#else
	return (((quint32)a)|(((quint32)b)<<8)|(((quint32)c)<<16)|(((quint32)d)<<24));
#endif
}

class VideoFormat {
public:
	typedef mp_imgfmt Type;
	VideoFormat(const mp_image *mpi, int dest_w, int dest_h): d(new Data(mpi, dest_w, dest_h)) {}
	VideoFormat(const QImage &image): d(new Data(image)) {}
	VideoFormat(): d(new Data) {}
	inline bool operator == (const VideoFormat &rhs) const {return d->compare(rhs.d.constData());}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return d->size;} // original pixel size
	inline QSize alignedSize() const {return d->alignedSize;} // aligned pixel size
	inline QSize outputSize() const {return d->outputSize;} // output pixel size
	inline bool isEmpty() const {return d->size.isEmpty() || d->type == IMGFMT_NONE;}
	inline double bps(double fps) const {return fps*_Area(d->size)*d->bpp;}
	inline Type type() const {return d->type;} // output type != d->imgfmt
	QString name() const;
	inline int bpp() const {return d->bpp;}
	inline int planes() const {return d->planes;}
	inline int width() const {return d->size.width();}
	inline int height() const {return d->size.height();}
	inline int alignedWidth() const {return d->alignedSize.width();}
	inline int alignedHeight() const {return d->alignedSize.height();}
	inline int bytesPerLine(int plane) const {return d->alignedByteSize[plane].width();}
	inline int lines(int plane) const {return d->alignedByteSize[plane].height();}
	inline bool compare(const mp_image *mpi) const {return d->compare(mpi);}
	inline bool isNative() const {return d->native;}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const mp_image *mpi, int dest_w, int dest_h);
		Data(const QImage &image);
		inline bool compare(const mp_image *mpi) const {
			return mpi->fmt.id == imgfmt && mpi->w == size.width() && mpi->h == size.height()
				&& (native || (alignedByteSize[0].width() == mpi->stride[0] && alignedByteSize[1].width() == mpi->stride[1] && alignedByteSize[2].width() == mpi->stride[2]));
		}
		inline bool compare(const Data *other) const {
			return type == other->type && size == other->size && alignedSize == other->alignedSize && outputSize == other->outputSize;
		}
		QSize size = {0, 0}, alignedSize = {0, 0}, outputSize = {0, 0};
		QVector<QSize> alignedByteSize = {3, QSize(0, 0)};
		int planes = 0, bpp = 0;
		Type type = IMGFMT_NONE, imgfmt = IMGFMT_NONE;
		bool native = false;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFORMAT_HPP
