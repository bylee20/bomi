#include "subtitledrawer.hpp"

SubCompImage::SubCompImage(const SubComp *comp, Iterator it, void *creator)
    : m_comp(comp)
    , m_it(it)
    , m_creator(creator)
{
    if (m_it != comp->end())
        m_text = *m_it;
}

SubCompImage::SubCompImage(const SubComp *comp)
    : m_comp(comp)
{
    if (comp)
        m_it = m_comp->end();
}

/******************************************************************************/

auto SubtitleDrawer::pos(const QSizeF &img, const QRectF &area) const -> QPointF
{
    QPointF pos(0.0, 0.0);
    const auto ih = img.height(), ah = area.height();
    if (m_alignment & Qt::AlignBottom)
        pos.ry() = qMax(0.0, ah*(1.0 - m_margin.bottom) - ih);
    else if (m_alignment & Qt::AlignVCenter)
        pos.ry() = (ah - ih)*0.5;
    else {
        pos.ry() = ah*m_margin.top;
        if (pos.y() + ih > ah)
            pos.ry() = ah - ih;
    }
    const auto iw = img.width(), aw = area.width();
    if (m_alignment & Qt::AlignHCenter)
        pos.rx() = (aw - iw)*0.5;
    else if (m_alignment & Qt::AlignRight)
        pos.rx() = aw*(1.0 - m_margin.right) - iw;
    else
        pos.rx() = aw*m_margin.left;
    pos += area.topLeft();
    return pos;
}

auto SubtitleDrawer::setStyle(const OsdStyle &style) -> void
{
    m_style = style;
    updateStyle(m_front, style);
    updateStyle(m_back, style);
    if (style.outline.enabled) {
        const auto size = style.font.height()*style.outline.width*2.0;
        m_back.setTextOutline(style.outline.color, size);
    } else
        m_back.setTextOutline(Qt::NoPen);
}

auto SubtitleDrawer::draw(QImage &image, int &gap, const RichTextDocument &text,
                          const QRectF &area, double dpr) -> QVector<QRectF>
{
    QVector<QRectF> bboxes;
    gap = 0;
    if (!(m_drawn = text.hasWords()))
        return bboxes;
    const double scale = this->scale(area)*dpr;
    const double fscale = m_style.font.height()*scale;
    auto make = [=] (RichTextDocument &doc, const RichTextDocument &from) {
        doc = from;
        doc += text;
        doc.updateLayoutInfo();
        doc.doLayout(area.width()/(scale/dpr));
    };
    RichTextDocument front, back;
    make(front, m_front);
    make(back, m_back);
    QPoint thick(0, 0);
    if (m_style.bbox.enabled)
        thick = (fscale*m_style.bbox.padding).toPoint();
    QPoint soffset(0, 0);
    if (m_style.shadow.enabled)
        soffset = (fscale*m_style.shadow.offset).toPoint();
    QPoint offset(0, 0);
    const int blur = m_style.shadow.blur ? qRound(fscale*0.01) : 0;
    const auto nsize = front.naturalSize()*scale;
    QSize imageSize(nsize.width() + 1, nsize.height() + 1);
    QPoint pad = soffset;
    if (soffset.x() < 0) {
        pad.rx() = offset.rx() = -soffset.x();
        soffset.rx() = 0;
    }
    if (soffset.y() < 0) {
        pad.ry() = offset.ry() = -soffset.y();
        soffset.ry() = 0;
    }
    imageSize += {pad.rx() + (blur+thick.x())*2, pad.ry() + (blur+thick.y())*2};
    const QPoint pblur(blur, blur);
    offset += pblur;
    offset += thick;
    image = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
    if (!image.isNull()) {
        const auto x = -(area.width() * dpr - nsize.width()) * 0.5 + offset.x();
        QPointF origin(x, offset.y());
        image.setDevicePixelRatio(dpr);
        image.fill(0x0);
        QPainter painter(&image);
        painter.translate(origin/dpr);
        painter.scale(scale/dpr, scale/dpr);
        back.draw(&painter, QPointF(0, 0));
        front.draw(&painter, QPointF(0, 0));
        painter.end();
        if (m_style.shadow.enabled) {
            QImage bg(image.size(), QImage::Format_ARGB32_Premultiplied);
            bg.setDevicePixelRatio(dpr);
            auto dest = bg.bits();
            const quint32 sr = m_style.shadow.color.red();
            const quint32 sg = m_style.shadow.color.green();
            const quint32 sb = m_style.shadow.color.blue();
            const quint32 sa = m_style.shadow.color.alpha();
            for (int y=0; y<bg.height(); ++y) {
                const int ys = y - soffset.y();
                if (ys < 0) {
                    memset(dest, 0, bg.bytesPerLine());
                    dest += bg.bytesPerLine();
                } else {
                    auto src = image.bits() + image.bytesPerLine() * ys;
                    for (int x=0; x<bg.width(); ++x) {
                        const int xs = x-soffset.x();
                        if (xs < 0) {
                            *dest++ = 0;
                            *dest++ = 0;
                            *dest++ = 0;
                            *dest++ = 0;
                        } else {
                            *dest++ = sb*sa*src[3] >> 16;
                            *dest++ = sg*sa*src[3] >> 16;
                            *dest++ = sr*sa*src[3] >> 16;
                            *dest++ = sa*   src[3] >> 8;
                            src += 4;
                        }
                    }
                }
            }
            if (blur)
                m_blur.applyTo(bg, m_style.shadow.color, blur);
            painter.begin(&bg);
            painter.drawImage(QPoint(0, 0), image);
            painter.end();
            image.swap(bg);
        }
        if (m_style.bbox.enabled) {
            bboxes = front.boundingBoxes();
            if (!bboxes.isEmpty()) {
                for (auto &bbox : bboxes) {
                    bbox.setTopLeft(bbox.topLeft() * scale + origin - thick);
                    bbox.setBottomRight(bbox.bottomRight() * scale
                                        + origin + thick);
                }
                gap = image.height() - bboxes.last().bottom() + 1;
                gap += thick.y()*2;
            }
        }
    }
    return bboxes;
}
