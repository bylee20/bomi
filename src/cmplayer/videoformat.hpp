#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include "stdafx.hpp"
extern "C" {
#include <video/mp_image.h>
#include <libavutil/pixfmt.h>
}
//struct mp_image;

//constexpr static inline quint32 cc4(char a, char b, char c, char d) {
//#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
//	return (((quint32)d)|(((quint32)c)<<8)|(((quint32)b)<<16)|(((quint32)a)<<24));
//#else
//	return (((quint32)a)|(((quint32)b)<<8)|(((quint32)c)<<16)|(((quint32)d)<<24));
//#endif
//}

class VideoFormat {
public:
	typedef mp_imgfmt Type;
	VideoFormat(const mp_image *mpi): d(new Data(mpi)) {}
	VideoFormat(const QImage &image): d(new Data(image)) {}
	VideoFormat(): d(new Data) {}
	inline bool operator == (const VideoFormat &rhs) const {return d->compare(rhs.d.constData());}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return d->size;} // original pixel size
	inline QSize alignedSize() const {return d->alignedSize;} // aligned pixel size
	inline QSize displaySize() const {return d->displaySize;} // display pixel size
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
	inline int bytesPerPlain(int plane) const { return bytesPerLine(plane)*lines(plane); }
	inline bool compare(const mp_image *mpi) const {return d->compare(mpi);}
	inline bool isNative() const {return d->native;}
	inline mp_csp colorspace() const { return d->colorspace; }
	inline mp_csp_levels range() const { return d->range; }
	inline Type imgfmt() const {return d->imgfmt;}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const mp_image *mpi);
		Data(const QImage &image);
		inline bool compare(const mp_image *mpi) const {
			return mpi->fmt.id == imgfmt && QSize(mpi->w, mpi->h) == size
					&& QSize(mpi->display_w, mpi->display_h) == displaySize
					&& mpi->colorspace == colorspace && mpi->levels == range
					&& (native || (alignedByteSize[0].width() == mpi->stride[0] && alignedByteSize[1].width() == mpi->stride[1] && alignedByteSize[2].width() == mpi->stride[2]));
		}
		inline bool compare(const Data *other) const {
			return colorspace == other->colorspace && range == other->range && type == other->type && size == other->size && alignedSize == other->alignedSize && displaySize == other->displaySize;
		}
		QSize size = {0, 0}, alignedSize = {0, 0}, displaySize = {0, 0};
		std::array<QSize, 3> alignedByteSize{{QSize(0, 0), QSize(0, 0), QSize(0, 0)}};
		int planes = 0, bpp = 0;
		Type type = IMGFMT_NONE, imgfmt = IMGFMT_NONE;
		bool native = false;
		mp_csp colorspace = MP_CSP_BT_601;
		mp_csp_levels range = MP_CSP_LEVELS_TV;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFORMAT_HPP
