#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include "stdafx.hpp"
extern "C" {
#include <video/mp_image.h>
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
	typedef quint32 Type;
	static constexpr quint32 Unknown = 0;
	static constexpr quint32 I420 = cc4('I', '4', '2', '0');
	static constexpr quint32 NV12 = cc4('N', 'V', '1', '2');
	static constexpr quint32 NV21 = cc4('N', 'V', '2', '1');
	static constexpr quint32 YUY2 = cc4('Y', 'U', 'Y', '2');
	static constexpr quint32 UYVY = cc4('U', 'Y', 'V', 'Y');
	static constexpr quint32 RGBA = cc4('R', 'G', 'B', 'A');
	static constexpr quint32 BGRA = cc4('B', 'G', 'R', 'A');
	static constexpr quint32 HWAC = cc4('H', 'W', 'A', 'C');
	VideoFormat(const mp_image *mpi): d(new Data(mpi)) {}
	VideoFormat(): d(new Data) {}
	inline bool operator == (const VideoFormat &rhs) const {return d->compare(rhs.d.constData());}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return d->size;}
	inline bool isEmpty() const {return d->size.isEmpty() || d->type == Unknown;}
	inline double bps(double fps) const {return fps*_Area(d->size)*d->bpp;}
	inline bool isYCbCr() const {return isYCbCr(d->type);}
	static inline bool isYCbCr(Type type) {return type != RGBA && type != BGRA && type != Unknown;}
	inline Type type() const {return d->type;}
	inline int bpp() const {return d->bpp;}
	inline int planes() const {return d->planes;}
	inline int width() const {return d->size.width();}
	inline int height() const {return d->size.height();}
	inline int drawWidth() const {return d->drawSize.width();}
	inline int drawHeight() const {return d->drawSize.height();}
	inline int byteWidth(int plane) const {return d->byteSize[plane].width();}
	inline int byteHeight(int plane) const {return d->byteSize[plane].height();}
	inline bool compare(const mp_image *mpi) const {return d->compare(mpi);}
	inline PixelFormat pixfmt() const {return d->pixfmt;}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const mp_image *mpi);
		inline bool compare(const mp_image *mpi) const {
			return mpi->fmt.id == imgfmt && mpi->w == size.width() && mpi->h == size.height()
				&& byteSize[0].width() && mpi->stride[0] && byteSize[1].width() && mpi->stride[1] && byteSize[2].width() && mpi->stride[2];
		}
		inline bool compare(const Data *other) const {
			return type == other->type && size == other->size && drawSize == other->drawSize;
		}
		QSize size = {0, 0}, drawSize = {0, 0};
		QVector<QSize> byteSize = {3, QSize(0, 0)};
		int planes = 0, bpp = 0, imgfmt = 0;
		quint32 type = Unknown;
		PixelFormat pixfmt = AV_PIX_FMT_NONE;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFORMAT_HPP
