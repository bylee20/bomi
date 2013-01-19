#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include "stdafx.hpp"

struct mp_image;

constexpr static inline quint32 cc4(char a, char b, char c, char d) {
#if (Q_BYTE_ORDER == Q_BIG_ENDIAN)
	return (((quint32)d)|(((quint32)c)<<8)|(((quint32)b)<<16)|(((quint32)a)<<24));
#else
	return (((quint32)a)|(((quint32)b)<<8)|(((quint32)c)<<16)|(((quint32)d)<<24));
#endif
}

static inline quint32 cc4(const QString &str) {
	return str.size() == 4 ? cc4(str[0].toLatin1(), str[1].toLatin1(), str[2].toLatin1(), str[3].toLatin1()) : 0;
}

struct VideoFormat {
	typedef quint32 Type;
	static constexpr quint32 Unknown = 0;
	static constexpr quint32 I420 = cc4('I', '4', '2', '0');
	static constexpr quint32 YV12 = cc4('Y', 'V', '1', '2');
	static constexpr quint32 NV12 = cc4('N', 'V', '1', '2');
	static constexpr quint32 NV21 = cc4('N', 'V', '2', '1');
	static constexpr quint32 YUY2 = cc4('Y', 'U', 'Y', '2');
	static constexpr quint32 UYVY = cc4('U', 'Y', 'V', 'Y');
	static constexpr quint32 RGBA = cc4('R', 'G', 'B', 'A');
	static constexpr quint32 BGRA = cc4('B', 'G', 'R', 'A');

	VideoFormat(quint32 type = Unknown): m_type(type) {}
	static VideoFormat fromMpImage(mp_image *mpi);
	inline bool operator == (const VideoFormat &rhs) const {
		return m_type == rhs.m_type && m_size == rhs.m_size && m_drawSize == rhs.m_drawSize;
	}
	inline bool operator != (const VideoFormat &rhs) const {return !operator == (rhs);}
	inline QSize size() const {return m_size;}
	inline bool isEmpty() const {return m_size.isEmpty() || m_type == Unknown;}
	inline double bps(double fps) const {return fps*_Area(m_size)*m_bpp;}
	bool isYCbCr() const {return isYCbCr(m_type);}
	static bool isYCbCr(Type type) {return type != RGBA && type != BGRA && type != Unknown;}
	Type type() const {return m_type;}
	int bpp() const {return m_bpp;}
	int planes() const {return m_planes;}
	int width() const {return m_size.width();}
	int height() const {return m_size.height();}
	int drawWidth() const {return m_drawSize.width();}
	QSize drawSize() const {return m_drawSize;}
	int drawHeight() const {return m_drawSize.height();}
	int byteWidth(int plane) const {return m_byteSize[plane].width();}
	int byteHeight(int plane) const {return m_byteSize[plane].height();}
	quint32 imgfmt() const {return m_imgfmt;}
private:
	QSize m_size = {0, 0}, m_drawSize = {0, 0};
	QVector<QSize> m_byteSize = {3, QSize(0, 0)};
	int m_planes = 0, m_bpp = 0;
	quint32 m_type = Unknown, m_imgfmt = -1;
};

VideoFormat::Type imgfmtToVideoFormatType(quint32 imgfmt);
uint32_t videoFormatTypeToImgfmt(VideoFormat::Type type);
static inline QString cc4ToString(uint32_t fourcc) {
	char str[5] = {0};	memcpy(str, &fourcc, 4);	return QString::fromLatin1(str);
}

QString cc4ToDescription(VideoFormat::Type type);


#endif // VIDEOFORMAT_HPP
