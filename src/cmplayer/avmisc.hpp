#ifndef AVMISC_HPP
#define AVMISC_HPP

#include <QtCore/QtGlobal>
#include <QtCore/QSize>
#include <QtCore/QString>

struct AudioFormat {
	int channels;
	int sample_rate;
};

constexpr static inline quint32 _f(char a, char b, char c, char d) {
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
	return (((quint32)d)|(((quint32)c)<<8)|(((quint32)b)<<16)|(((quint32)a)<<24));
#else
	return (((quint32)a)|(((quint32)b)<<8)|(((quint32)c)<<16)|(((quint32)d)<<24));
#endif
}

static inline quint32 _f(const QString &str) {
	return str.size() == 4 ? _f(str[0].toAscii(), str[1].toAscii(), str[2].toAscii(), str[3].toAscii()) : 0;
}

struct VideoFormat {
	enum Type : quint32 {
		Unknown = 0,
		I420	= _f('I', '4', '2', '0'),
		YV12	= _f('Y', 'V', '1', '2'),
		NV12	= _f('N', 'V', '1', '2'),
		NV21	= _f('N', 'V', '2', '1'),
		YUY2	= _f('Y', 'U', 'Y', '2')

	};
	VideoFormat() {
		planes = pitch = bpp = width = height = stride = 0;
	}
	inline bool isCompatibleWith(const VideoFormat &other) const {
		return planes == other.planes && type == other.type && bpp == other.bpp && other.height < height && other.stride < stride;
	}
	inline bool operator == (const VideoFormat &rhs) const {
		return planes == rhs.planes && type == rhs.type && bpp == rhs.bpp && width == rhs.width && height == rhs.height && stride == rhs.stride && pitch == rhs.pitch;
	}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return QSize(width, height);}
	inline bool isEmpty() const {return width < 1 || height < 1;}
	inline double bps(double fps) const {return fps*width*height*bpp;}
	Type type = Unknown;
	int planes;
	int bpp, width, height, stride, pitch;
};

uint32_t _fToImgFmt(VideoFormat::Type type);
static inline QString _fToString(uint32_t fourcc) {
	char str[5] = {0};	memcpy(str, &fourcc, 4);	return QString::fromAscii(str);
}

QString _fToDescription(VideoFormat::Type type);

#endif // AVMISC_HPP
