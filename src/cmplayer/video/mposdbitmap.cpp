#include "mposdbitmap.hpp"
#include "tmp/static_op.hpp"
extern "C" {
#include <sub/osd.h>
}

namespace OGL { auto maximumTextureSize() -> int; }

MpOsdBitmap::Id::Id(const sub_bitmaps *bitmaps) noexcept
    : image(bitmaps->bitmap_id)
    , pos(bitmaps->bitmap_pos_id) { }

auto MpOsdBitmap::Id::isSame(const sub_bitmaps *bitmaps) const -> bool
{
    return image == bitmaps->bitmap_id && pos == bitmaps->bitmap_pos_id;
}

auto MpOsdBitmap::copy(const sub_bitmaps *imgs, const QSize &renderSize) -> bool
{
    if (m_id == Id(imgs))
        return false;
    m_count = imgs->num_parts;
    m_renderSize = renderSize;
    if (m_parts.size() < imgs->num_parts)
        _Expand(m_parts, imgs->num_parts);

//                                         none      ass  rgba  indexed
    static constexpr int corrections[] = { 0,        1,   1,    4    };
    static constexpr Format     fmts[] = { NoFormat, Ass, Rgba, Rgba };
    static constexpr int      shifts[] = { 0,        0,   2,    2    };
    static const int max = OGL::maximumTextureSize();

    const int c_stride = corrections[imgs->format];
    const int shift = shifts[imgs->format];
    m_format = fmts[imgs->format];
    m_id = { imgs };
    m_parts.resize(imgs->num_parts);
    m_sheet = {0, 0};

    int offset = 0, lineHeight = 0;
    QPoint map{0, 0};
    for (int i=0; i<imgs->num_parts; ++i) {
        auto &img = imgs->parts[i];
        auto &part = m_parts[i];
        part.m_size = { img.w, img.h };
        part.m_display = { img.x, img.y, img.dw, img.dh };
        if (imgs->format == SUBBITMAP_LIBASS) {
            const quint32 color = img.libass.color;
            part.m_color = (color & 0xffffff00) | (0xff - (color & 0xff));
        }
        part.m_stride = tmp::aligned<4>(img.stride * c_stride);
        part.m_strideAsPixel = (part.m_stride >> shift);
        part.m_offset = offset;
        offset += part.m_stride * part.height();

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
    _Expand(m_data, offset);

    std::array<quint32, 256> palette;
    for (int i=0; i<imgs->num_parts; ++i) {
        auto &img = imgs->parts[i];
        auto &part = m_parts[i];
        if (imgs->format == SUBBITMAP_INDEXED) {
            auto bmp = static_cast<osd_bmp_indexed*>(img.bitmap);
            for (size_t i = 0; i < palette.size(); ++i) {
                const quint32 c = bmp->palette[i];
                const quint32 a =   c >> 24 & 0xff;
                const quint32 r = ((c >> 16 & 0xff) * a) >> 8;
                const quint32 g = ((c >> 8  & 0xff) * a) >> 8;
                const quint32 b = ((c >> 0  & 0xff) * a) >> 8;
                palette[i] = (a << 24) | (r << 16) | (g << 8) | b;
            }
            quint32 *p = data<quint32>(i);
            for (int y=0; y<img.h; ++y) {
                quint32 *dest = p + y * part.strideAsPixel();
                const uchar *src = bmp->bitmap + y * img.stride;
                for (int x = 0; x < img.w; ++x)
                    *dest++ = palette[*src++];
            }
        } else {
            Q_ASSERT(img.stride > 0 && img.stride/4 < 10000);
            if (part.m_stride == img.stride)
                memcpy(data(i), img.bitmap, img.stride*img.h);
            else {
                auto dest = data(i);
                auto src = static_cast<uchar*>(img.bitmap);
                for (int i = 0; i < img.h; ++i) {
                    memcpy(dest, src, img.w);
                    dest += part.m_stride;
                    src  += img.stride;
                }
            }
        }
    }
    return true;
}

auto MpOsdBitmap::toImage() const -> QImage
{
    auto fmt = QImage::Format_ARGB32_Premultiplied;
    QImage frame(m_renderSize, fmt);
    frame.fill(0x0);
    QPainter painter(&frame);
    if (m_format & Rgba) {
        for (int i=0; i<m_parts.size(); ++i) {
            auto &part = m_parts[i];
            const QImage image(data(i), part.strideAsPixel(),
                               part.height(), fmt);
            painter.drawImage(part.display(), image,
                              QRect(0, 0, part.width(), part.height()));
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
                    const quint32 a = (part.color() & 0xff) * int(*src++)/255;
                    const quint32 r = part.color() >> 24 & 0xff;
                    const quint32 g = part.color() >> 16 & 0xff;
                    const quint32 b = part.color() >> 8  & 0xff;
                    *dest++ = qRgba(r*a/255, g*a/255, b*a/255, a);
                }
            }
            painter.drawImage(part.display(), image,
                              QRect(0, 0, part.width(), part.height()));
        }
    }
    return frame;
}

auto MpOsdBitmap::drawOn(QImage &frame) const -> void
{
    QPainter painter(&frame);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.scale(frame.width() /(double)m_renderSize.width(),
                  frame.height()/(double)m_renderSize.height());
    auto fmt = QImage::Format_ARGB32_Premultiplied;
    if (m_format & Rgba) {
        for (int i=0; i<m_parts.size(); ++i) {
            auto &part = m_parts[i];
            const QImage image(data(i), part.strideAsPixel(),
                               part.height(), fmt);
            painter.drawImage(part.display(), image,
                              QRect(0, 0, part.width(), part.height()));
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
                    const quint32 a = (part.color() & 0xff) * int(*src++)/255;
                    const quint32 r = part.color() >> 24 & 0xff;
                    const quint32 g = part.color() >> 16 & 0xff;
                    const quint32 b = part.color() >> 8  & 0xff;
                    *dest++ = qRgba(r*a/255, g*a/255, b*a/255, a);
                }
            }
            painter.drawImage(part.display(), image,
                              QRect(0, 0, part.width(), part.height()));
        }
    }
}
