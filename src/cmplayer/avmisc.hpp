#ifndef AVMISC_HPP
#define AVMISC_HPP

#include <QtCore/QtGlobal>
#include <QtCore/QSize>
#include <QtCore/QString>

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
		YUY2	= _f('Y', 'U', 'Y', '2'),
		RGBA	= _f('R', 'G', 'B', 'A'),
		BGRA	= _f('B', 'G', 'R', 'A')
	};
	VideoFormat() {}
	static VideoFormat fromType(Type type, int width, int height);
	static VideoFormat fromImgFmt(uint32_t imgfmt, int width, int height);
#define CHECK(a) (a == rhs.a)
	inline bool isCompatibleWith(const VideoFormat &rhs) const {
		return CHECK(type) && rhs.height < height && rhs.stride < stride;
	}
	inline bool operator == (const VideoFormat &rhs) const {
		return CHECK(type) && CHECK(stride) && CHECK(width) && CHECK(height);
	}
#undef CHECK
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return QSize(width, height);}
	inline bool isEmpty() const {return width < 1 || height < 1;}
	inline double bps(double fps) const {return fps*width*height*bpp;}
	bool planar = false;
	int planes = 0, bpp = 0, width = 0, height = 0, stride = 0, width_stride = 0;
	Type type = Unknown;
};

uint32_t _fToImgFmt(VideoFormat::Type type);
static inline QString _fToString(uint32_t fourcc) {
	char str[5] = {0};	memcpy(str, &fourcc, 4);	return QString::fromAscii(str);
}

QString _fToDescription(VideoFormat::Type type);

#endif // AVMISC_HPP
