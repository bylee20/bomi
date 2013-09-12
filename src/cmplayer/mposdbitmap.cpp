#include "mposdbitmap.hpp"
#include "openglcompat.hpp"
extern "C" {
#include <sub/sub.h>
}

bool MpOsdBitmap::copy(const sub_bitmaps *imgs, const QSize &renderSize) {
	if (imgs->num_parts <= 0 || (id == imgs->bitmap_id && pos == imgs->bitmap_pos_id))
		return false;
	m_renderSize = renderSize;
	if (m_size < imgs->num_parts) {
		m_size = imgs->num_parts;
		const int count = m_size*1.5;
		m_parts.resize(count);
	}

	id = imgs->bitmap_id;
	pos = imgs->bitmap_pos_id;
	m_parts.resize(imgs->num_parts);
	m_sheet = {0, 0};
	int shift = 2;

	int c_stride = 1;
	switch (imgs->format) {
	case SUBBITMAP_INDEXED:
		c_stride = 4;
		m_format = Rgba;
		break;
	case SUBBITMAP_RGBA:
		m_format = RgbaPA;
		break;
	case SUBBITMAP_LIBASS:
		m_format = Ass;
		shift = 0;
		break;
	default:
		return false;
	}

	int offset = 0, lineHeight = 0;
	QPoint map{0, 0};
	static const int max = OpenGLCompat::maximumTextureSize();
	for (int i=0; i<imgs->num_parts; ++i) {
		auto &img = imgs->parts[i];
		auto &part = m_parts[i];
		part.m_size = {img.w, img.h};
		part.m_display = {img.x, img.y, img.dw, img.dh};
		if (imgs->format == SUBBITMAP_LIBASS) {
			const quint32 color = img.libass.color;
			part.m_color = (color & 0xffffff00) | (0xff - (color & 0xff));
		}
		part.m_stride = _Aligned<4>(img.stride*c_stride);
		part.m_strideAsPixel = part.m_stride >> shift;
		part.m_offset = offset;
		offset += part.m_stride*part.height();

		if (part.strideAsPixel() > m_maximumSize.width())
			m_maximumSize.rwidth() = part.strideAsPixel();
		if (part.size().height() > m_maximumSize.height())
			m_maximumSize.rheight() = part.size().height();
		if (map.x() + part.strideAsPixel() > max) {
			map.rx() = 0; map.ry() += lineHeight;
			lineHeight = 0;
		}
		part.m_map = map;
		if (part.size().height() > lineHeight)
			lineHeight = part.size().height();
		map.rx() += part.strideAsPixel();
		if (map.x() > m_sheet.width())
			m_sheet.rwidth() = map.x();
	}
	m_sheet.rheight() = map.y() + lineHeight;
	if (m_data.size() < offset)
		m_data.resize(offset);
	for (int i=0; i<imgs->num_parts; ++i) {
		auto &img = imgs->parts[i];
		auto &part = m_parts[i];
		if (imgs->format == SUBBITMAP_INDEXED) {
			auto bmp = static_cast<osd_bmp_indexed*>(img.bitmap);
			quint32 *p = data<quint32>(i);
			for (int y=0; y<img.h; ++y) {
				quint32 *dest = p + y*part.strideAsPixel();
				const uchar *src = bmp->bitmap + y*img.stride;
				for (int x=0; x<img.w; ++x)
					*dest++ = bmp->palette[*src++];
			}
		} else {
			if (part.m_stride == img.stride)
				memcpy(data(i), img.bitmap, img.stride*img.h);
			else {
				auto dest = data(i);
				auto src = static_cast<uchar*>(img.bitmap);
				for (int i=0; i<img.h; ++i, dest += part.m_stride, src += img.stride)
					memcpy(dest, src, img.w);
			}
		}
	}
	return true;
}

void MpOsdBitmap::drawOn(QImage &frame) const {
	QPainter painter(&frame);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.scale((double)frame.width()/(double)m_renderSize.width(), (double)frame.height()/(double)m_renderSize.height());
	auto fmt = m_format & PaMask ? QImage::Format_ARGB32_Premultiplied : QImage::Format_ARGB32;
	if (m_format & Rgba) {
		for (int i=0; i<m_parts.size(); ++i) {
			auto &part = m_parts[i];
			const QImage image(data(i), part.strideAsPixel(), part.height(), fmt);
			painter.drawImage(part.display(), image, QRect(0, 0, part.width(), part.height()));
		}
	} else {
		Q_ASSERT(m_format == Ass);
		for (int i=0; i<m_parts.size(); ++i) {
			auto &part = m_parts[i];
			QImage image(part.size(), fmt);
			for (int y=0; y<part.height(); ++y) {
				auto dest = reinterpret_cast<QRgb*>(image.scanLine(y));
				auto src = data(i) + y*part.m_stride;
				for (int x=0; x<part.width(); ++x) {
					const int a = (part.color() & 0xff)*static_cast<int>(*src++)/255;
					const int r = part.color() >> 24 & 0xff;
					const int g = part.color() >> 16 & 0xff;
					const int b = part.color() >> 8  & 0xff;
					*dest++ = qRgba(r*a/255, g*a/255, b*a/255, a);
				}
			}
			painter.drawImage(part.display(), image, QRect(0, 0, part.width(), part.height()));
		}
	}
}
