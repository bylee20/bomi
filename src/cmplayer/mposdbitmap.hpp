#ifndef MPOSDBITMAP_HPP
#define MPOSDBITMAP_HPP

#include "stdafx.hpp"

struct sub_bitmaps;

class MpOsdBitmap {
public:
	enum Format {
		PaMask = 0x01, /*Premultipled Alpha*/
		Ass = 0x10 | PaMask,
		Rgba = 0x20,
		RgbaPA = 0x20 | PaMask
	};
	struct PartInfo {
		const QRect &display() const { return m_display; }
		const QSize &size() const { return m_size; }
		const QPoint &map() const { return m_map; }
		quint32 color() const { return m_color; }
		int strideAsPixel() const { return m_strideAsPixel; }
		int width() const { return size().width(); }
		int height() const { return size().height(); }
	private:
		friend class MpOsdBitmap;
		QRect m_display = {0, 0, 0, 0};
		QSize m_size = {0, 0};
		QPoint m_map = {0, 0};
		quint32 m_color = 0;
		int m_stride = 0, m_offset = 0, m_strideAsPixel = 0;
	};
	bool copy(const sub_bitmaps *imgs, const QSize &renderSize);
	template<typename T = uchar>
	T *data(int i) { return (T*)(m_data.data() + m_parts[i].m_offset); }
	template<typename T = uchar>
	const T *data(int i) const { return (const T*)(m_data.data() + m_parts[i].m_offset); }
	int count() const { return m_size; }
	const PartInfo &part(int i) const { return m_parts[i]; }
	Format format() const { return m_format; }
	const QSize &renderSize() const { return m_renderSize; }
	const QSize &sheet() const { return m_sheet; }
	void drawOn(QImage &frame) const;
private:
	QByteArray m_data;
	int m_size = 0, id = -1, pos = -1;
	QVector<PartInfo> m_parts;
	Format m_format = RgbaPA;
	QSize m_sheet = {0, 0}, m_maximumSize = {0, 0}, m_renderSize = {0, 0};
};

#endif // MPOSDBITMAP_HPP
