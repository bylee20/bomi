#ifndef SUBTITLEDRAWER_HPP
#define SUBTITLEDRAWER_HPP

#include "misc/osdstyle.hpp"
#include "subtitle.hpp"

struct Margin {
    Margin() {}
    Margin(const QPointF &tl, const QPointF &br)
        : top(tl.y()), right(br.x()), bottom(br.y()), left(tl.x()) {}
    double top = 0.0, right = 0.0, bottom = 0.0, left = 0.0;
};

class FastAlphaBlur {
public:
    FastAlphaBlur(): radius(-1) {}
    // copied from openframeworks superfast blur and modified
    auto applyTo(QImage &mask, const QColor &color, int radius) -> void {
        if (radius < 1 || mask.isNull())
            return;
        setSize(mask.size());
        setRadius(radius);
        const int w = s.width();
        const int h = s.height();

        uchar *a = valpha.data();
        const uchar *inv = vinv.constData();
        int *min = vmin.data();
        int *max = vmax.data();

        const int xmax = mask.width()-1;
        for (int x=0; x<w; ++x) {
            min[x] = qMin(x + radius + 1, xmax);
            max[x] = qMax(x - radius, 0);
        }

        const uchar *c_bits = mask.constBits()+3;
        uchar *it = a;
        for (int y=0; y<h; ++y, c_bits += (mask.width() << 2)) {
            int sum = 0;
            for(int i=-radius; i<=radius; ++i)
                sum += c_bits[qBound(0, i, xmax) << 2];
            for (int x=0; x<w; ++x, ++it) {
                sum += c_bits[min[x] << 2];
                sum -= c_bits[max[x] << 2];
                *it = inv[sum];
            }
        }

        const int ymax = mask.height()-1;
        for (int y=0; y<h; ++y){
            min[y] = qMin(y + radius + 1, ymax)*w;
            max[y] = qMax(y - radius, 0)*w;
        }

        uchar *bits = mask.bits();
        const double r = color.redF();
        const double g = color.greenF();
        const double b = color.blueF();
        const uchar *c_it = a;
        for (int x=0; x<w; ++x, ++c_it){
            int sum = 0;
            int yp = -radius*w;
            for(int i=-radius; i<=radius; ++i, yp += w)
                sum += c_it[qMax(0, yp)];
            uchar *p = bits + (x << 2);
            for (int y=0; y<h; ++y, p += (xmax << 2)){
                const uchar a = inv[sum];
                if (p[3] < 255) {
                    *p++ = a*b;
                    *p++ = a*g;
                    *p++ = a*r;
                    *p++ = a;
                } else {
                    p += 4;
                }
                sum += c_it[min[y]];
                sum -= c_it[max[y]];
            }
        }
    }

private:
    auto setSize(const QSize &size) -> void {
        if (size != s) {
            s = size;
            if (!s.isEmpty()) {
                vmin.resize(qMax(s.width(), s.height()));
                vmax.resize(vmin.size());
                valpha.resize(s.width()*s.height());
            }
        }
    }
    auto setRadius(const int radius) -> void {
        if (this->radius != radius) {
            this->radius = radius;
            if (radius > 0) {
                const int range = (radius << 1) + 1;
                vinv.resize(range << 8);
                for (int i=0; i<vinv.size(); ++i)
                    vinv[i] = i/range;
            }
        }
    }
    int radius;
    QSize s;
    QVector<int> vmin, vmax;
    QVector<uchar> valpha, vinv;
};

class SubCompImage : public QImage {
    using Iterator = SubComp::const_iterator;
public:
    SubCompImage(const SubComp *comp, Iterator it, void *creator);
    SubCompImage(const SubComp *comp);
    auto iterator() const -> Iterator { return m_it; }
    auto text() const -> const RichTextDocument& { return m_text; }
    auto component() const -> const SubComp* { return m_comp; }
    auto layoutSize() const -> QSize { return size()/devicePixelRatio(); }
    auto isValid() const -> bool { return m_comp && m_it != m_comp->end(); }
    auto creator() const -> void* { return m_creator; }
    auto boundingBoxes() const -> const QVector<QRectF>& { return m_bboxes; }
    auto gap() const -> int { return m_gap; }
private:
    friend class SubtitleDrawer;
    const SubComp *m_comp = nullptr;
    Iterator m_it;
    RichTextDocument m_text;
    QVector<QRectF> m_bboxes;
    int m_gap = 0;
    void *m_creator = nullptr;
};

class SubtitleDrawer {
public:
    auto setStyle(const OsdStyle &style) -> void;
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setMargin(const Margin &margin) -> void { m_margin = margin; }
    auto hasDrawn() const -> bool {return m_drawn;}
    auto draw(QImage &image, int &gap, const RichTextDocument &text,
              const QRectF &area, double dpr = 1.0) -> QVector<QRectF>;
    auto draw(SubCompImage &pic, const QRectF &area, double dpr = 1.0) -> bool;
    auto pos(const QSizeF &image, const QRectF &area) const -> QPointF;
    auto alignment() const -> Qt::Alignment { return m_alignment; }
    auto margin() const -> const Margin& { return m_margin; }
    auto style() const -> const OsdStyle& {return m_style;}
    auto scale(const QRectF &area) const -> double;
private:
    static auto updateStyle(RichTextDocument &doc,
                            const OsdStyle &style) -> void;
    OsdStyle m_style;
    RichTextDocument m_front, m_back;
    Margin m_margin;
    Qt::Alignment m_alignment;
    bool m_drawn = false;
    FastAlphaBlur m_blur;
    QByteArray m_buffer;
};

inline auto SubtitleDrawer::setAlignment(Qt::Alignment alignment) -> void
{
    m_back.setAlignment(m_alignment = alignment);
    m_front.setAlignment(alignment);
}

inline auto SubtitleDrawer::draw(SubCompImage &pic, const QRectF &area,
                                 double dpr) -> bool
{
    pic.m_bboxes = draw(pic, pic.m_gap, pic.m_text, area, dpr);
    return !pic.isNull();
}

SIA _Diagonal(double w, double h) -> double
{ return sqrt(w * w + h * h); }

SIA _Diagonal(const QSizeF &size) -> double
{ return _Diagonal(size.width(), size.height()); }

inline auto SubtitleDrawer::scale(const QRectF &area) const -> double
{
    double px = m_style.font.size;
//    if (policy == OsdStyle::Font::Scale::Diagonal)
//        px *= _Diagonal(area.size());
//    else if (policy == OsdStyle::Font::Scale::Width)
//        px *= area.width();
//    else
        px *= area.height();
    return px/m_style.font.height();
}

inline auto SubtitleDrawer::updateStyle(RichTextDocument &doc,
                                        const OsdStyle &style) -> void {
    doc.setFontPixelSize(style.font.height());
    doc.setWrapMode(style.wrapMode);
    doc.setFormat(QTextFormat::ForegroundBrush, QBrush(style.font.color));
    doc.setFormat(QTextFormat::FontFamily, style.font.family());
    doc.setFormat(QTextFormat::FontUnderline, style.font.underline());
    doc.setFormat(QTextFormat::FontStrikeOut, style.font.strikeOut());
    doc.setFormat(QTextFormat::FontWeight, style.font.weight());
    doc.setFormat(QTextFormat::FontItalic, style.font.italic());
    doc.setLeading(style.spacing.line, style.spacing.paragraph);
}

#endif // SUBTITLEDRAWER_HPP
