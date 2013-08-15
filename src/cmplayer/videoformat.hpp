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
	static constexpr quint32 VAGL = cc4('V', 'A', 'G', 'L'); // VA-API OpenGL type; same as BGRA
	VideoFormat(const mp_image *mpi, int dest_w, int dest_h): d(new Data(mpi, dest_w, dest_h)) {}
	VideoFormat(const QImage &image): d(new Data(image)) {}
	VideoFormat(): d(new Data) {}
	inline bool operator == (const VideoFormat &rhs) const {return d->compare(rhs.d.constData());}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return d->size;} // original pixel size
	inline QSize alignedSize() const {return d->alignedSize;} // aligned pixel size
	inline QSize outputSize() const {return d->outputSize;} // output pixel size
	inline bool isEmpty() const {return d->size.isEmpty() || d->type == Unknown;}
	inline double bps(double fps) const {return fps*_Area(d->size)*d->bpp;}
	inline bool isYCbCr() const {return isYCbCr(d->type);}
	static inline bool isYCbCr(Type type) {return type != VAGL && type != RGBA && type != BGRA && type != Unknown;}
	inline Type type() const {return d->type;}
	inline int bpp() const {return d->bpp;}
	inline int planes() const {return d->planes;}
	inline int width() const {return d->size.width();}
	inline int height() const {return d->size.height();}
	inline int alignedWidth() const {return d->alignedSize.width();}
	inline int alignedHeight() const {return d->alignedSize.height();}
	inline int bytesPerLine(int plane) const {return d->alignedByteSize[plane].width();}
	inline int lines(int plane) const {return d->alignedByteSize[plane].height();}
	inline bool compare(const mp_image *mpi) const {return d->compare(mpi);}
	inline AVPixelFormat pixfmt() const {return d->pixfmt;}
	inline GLenum glFormat() const {return d->glFormat;}
private:
	struct Data : public QSharedData {
		Data() {}
		Data(const mp_image *mpi, int dest_w, int dest_h);
		Data(const QImage &image);
		inline bool compare(const mp_image *mpi) const {
			return mpi->fmt.id == imgfmt && mpi->w == size.width() && mpi->h == size.height()
				&& alignedByteSize[0].width() == mpi->stride[0] && alignedByteSize[1].width() == mpi->stride[1] && alignedByteSize[2].width() == mpi->stride[2];
		}
		inline bool compare(const Data *other) const {
			return type == other->type && size == other->size && alignedSize == other->alignedSize && outputSize == other->outputSize;
		}
		QSize size = {0, 0}, alignedSize = {0, 0}, outputSize = {0, 0};
		QVector<QSize> alignedByteSize = {3, QSize(0, 0)};
		int planes = 0, bpp = 0, imgfmt = 0;
		quint32 type = Unknown;
		GLenum glFormat = GL_NONE;
		AVPixelFormat pixfmt = AV_PIX_FMT_NONE;
	};
	QSharedDataPointer<Data> d;
};

#endif // VIDEOFORMAT_HPP
