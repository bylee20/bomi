#include "subtitledrawer.hpp"

QPointF SubtitleDrawer::pos(const QSizeF &image, const QRectF &area) const {
    QPointF pos(0.0, 0.0);
    if (m_alignment & Qt::AlignBottom) {
        pos.ry() = qMax(0.0, area.height()*(1.0 - m_margin.bottom) - image.height());
    } else if (m_alignment & Qt::AlignVCenter)
        pos.ry() = (area.height() - image.height())*0.5;
    else {
        pos.ry() = area.height()*m_margin.top;
        if (pos.y() + image.height() > area.height())
            pos.ry() = area.height() - image.height();
    }
    if (m_alignment & Qt::AlignHCenter)
        pos.rx() = (area.width() - image.width())*0.5;
    else if (m_alignment & Qt::AlignRight)
        pos.rx() = area.width()*(1.0 - m_margin.right) - image.width();
    else
        pos.rx() = area.width()*m_margin.left;
    pos += area.topLeft();
    return pos;
}

void SubtitleDrawer::setStyle(const SubtitleStyle &style) {
    m_style = style;
    updateStyle(m_front, style);
    updateStyle(m_back, style);
    if (style.outline.enabled)
        m_back.setTextOutline(style.outline.color, style.font.height()*style.outline.width*2.0);
    else
        m_back.setTextOutline(Qt::NoPen);
}

QVector<QRectF> SubtitleDrawer::draw(QImage &image, int &gap, const RichTextDocument &text, const QRectF &area, double dpr) {
    QVector<QRectF> bboxes;
    gap = 0;
    if (!(m_drawn = text.hasWords()))
        return bboxes;
    const double scale = this->scale(area)*dpr;
    const double fscale = m_style.font.height()*scale;
    auto make = [this, text, scale, area, dpr] (RichTextDocument &doc, const RichTextDocument &from) {
        doc = from; doc += text; doc.updateLayoutInfo(); doc.doLayout(area.width()/(scale/dpr));
    };
    RichTextDocument front, back; make(front, m_front); make(back, m_back);
    QPoint thick = m_style.bbox.enabled ? (fscale*m_style.bbox.padding).toPoint() : QPoint(0, 0);
    QPoint soffset = m_style.shadow.enabled ? (fscale*m_style.shadow.offset).toPoint() : QPoint(0, 0);
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
        QPointF origin(-(area.width()*dpr - nsize.width())*0.5 + offset.x(), offset.y());
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
                    auto src = image.bits() + image.bytesPerLine()*ys;
                    for (int x=0; x<bg.width(); ++x) {
                        const int xs = x-soffset.x();
                        if (xs < 0) {
                            *dest++ = 0;
                            *dest++ = 0;
                            *dest++ = 0;
                            *dest++ = 0;
                        } else {
                            *dest++ = sr*sa*src[3] >> 16;
                            *dest++ = sg*sa*src[3] >> 16;
                            *dest++ = sb*sa*src[3] >> 16;
                            *dest++ = sa*   src[3] >> 8;
                            src += 4;
                        }
                    }
                }
            }
            if (blur)
                m_blur.applyTo(bg, Qt::black, blur);
            painter.begin(&bg);
            painter.drawImage(QPoint(0, 0), image);
            painter.end();
            image.swap(bg);
        }
        if (m_style.bbox.enabled) {
            bboxes = front.boundingBoxes();
            if (!bboxes.isEmpty()) {
                for (auto &bbox : bboxes) {
                    bbox.setTopLeft(bbox.topLeft()*scale + origin - thick);
                    bbox.setBottomRight(bbox.bottomRight()*scale + origin + thick);
                }
                gap = image.height() - bboxes.last().bottom() + 1;
                gap += thick.y()*2;
            }
        }
    }
    return bboxes;
}
