#ifndef AVMISC_HPP
#define AVMISC_HPP

#include <QtCore/QtGlobal>
#include <QtCore/QSize>
#include <QtCore/QString>

#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
#define FOURCC(a, b, c, d) \
	(((quint32)d)|(((quint32)c)<<8)|(((quint32)b)<<16)|(((quint32)a)<<24))
#else
#define FOURCC(a, b, c, d) \
	(((quint32)a)|(((quint32)b)<<8)|(((quint32)c)<<16)|(((quint32)d)<<24))
#endif

struct AudioBuffer {
	void *block;
	void *data;
	int samples;
	int size;
};

struct AudioUtil {
	void *af;
	AudioBuffer *(*allocBuffer)(void *af, int size);
	void (*freeBuffer)(void *af, AudioBuffer *buffer);
	int scaletempo_enabled;
};

struct AudioFormat {
	int channels;
	int sample_rate;
};

#define VIDEO_FRAME_MAX_PLANE_COUNT (3)

struct VideoUtil {
	void *vd;
	void (*mousePresseEvent)(void *vd, int button);
	void (*mouseReleaseEvent)(void *vd, int button);
	void (*mouseMoveEvent)(void *vd, int x, int y);
};

struct FramePlane {
	int data_pitch, data_lines;
	int frame_pitch, frame_lines;
	FramePlane(): data_pitch(0), data_lines(0), frame_pitch(0), frame_lines(0) {}
};

struct VideoFormat {
	quint32 source_fourcc;
	quint32 output_fourcc;
	int source_bpp;
	int output_bpp;
	int width;
	int height;
	FramePlane planes[VIDEO_FRAME_MAX_PLANE_COUNT];
	int plane_count;
	double sar;
	double fps;
	VideoFormat() {}
	VideoFormat(const VideoFormat &rhs)
	: source_fourcc(rhs.source_fourcc), output_fourcc(rhs.output_fourcc)
	, source_bpp(rhs.source_bpp), output_bpp(rhs.output_bpp)
	, width(rhs.width), height(rhs.height), plane_count(rhs.plane_count)
	, sar(rhs.sar), fps(rhs.fps) {
		planes[0] = rhs.planes[0];
		planes[1] = rhs.planes[1];
		planes[2] = rhs.planes[2];
	}
	VideoFormat &operator = (const VideoFormat &rhs) {
		if (this != &rhs) {
			source_fourcc = rhs.source_fourcc;
			output_fourcc = rhs.output_fourcc;
			source_bpp = rhs.source_bpp;
			output_bpp = rhs.output_bpp;
			width = rhs.width;
			height = rhs.height;
			planes[0] = rhs.planes[0];
			planes[1] = rhs.planes[1];
			planes[2] = rhs.planes[2];
			plane_count = rhs.plane_count;
			sar = rhs.sar;
			fps = rhs.fps;
		}
		return *this;
	}
	enum Type {
		Unknown = 0,
		I420	= FOURCC('I', '4', '2', '0'),
		YV12	= FOURCC('Y', 'V', '1', '2'),
		RV16	= FOURCC('R', 'V', '1', '6')
	};
	inline QSize size() const {return QSize(width, height);}
	static inline bool isPlanar(quint32 fourcc) {return fourcc == I420 || fourcc == YV12;}
	static inline bool toType(quint32 fourcc) {
		switch (fourcc) {
		case I420:
		case YV12:
		case RV16:
			return (Type)fourcc;
		default:
			return Unknown;
		}
	}
	static inline QString fourccToString(quint32 fcc) {
		char str[5] = {0};	memcpy(str, &fcc, 4);
		return QString::fromAscii(str);
	}
};

#endif // AVMISC_HPP
