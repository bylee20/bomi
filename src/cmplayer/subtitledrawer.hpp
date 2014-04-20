#ifndef SUBTITLEDRAWER_HPP
#define SUBTITLEDRAWER_HPP

#include "richtextdocument.hpp"
#include "subtitlestyle.hpp"
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
    void applyTo(QImage &mask, const QColor &color, int radius) {
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
        const double coef = color.alphaF();
        const double r = color.redF()*coef;
        const double g = color.greenF()*coef;
        const double b = color.blueF()*coef;
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
                    *p++ = a*coef;
                } else {
                    p += 4;
                }
                sum += c_it[min[y]];
                sum -= c_it[max[y]];
            }
        }
    }

private:
    void setSize(const QSize &size) {
        if (size != s) {
            s = size;
            if (!s.isEmpty()) {
                vmin.resize(qMax(s.width(), s.height()));
                vmax.resize(vmin.size());
                valpha.resize(s.width()*s.height());
            }
        }
    }
    void setRadius(const int radius) {
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
public:
    SubCompImage(const SubComp *comp, SubComp::const_iterator it, void *creator)
    : m_comp(comp), m_it(it), m_creator(creator) { if (m_it != comp->end()) m_text = *m_it; }
    SubCompImage(const SubComp *comp): m_comp(comp) { if (comp) m_it = m_comp->end(); }
    SubComp::const_iterator iterator() const { return m_it; }
    const RichTextDocument &text() const { return m_text; }
    const SubComp *component() const { return m_comp; }
    QSize layoutSize() const { return size()/devicePixelRatio(); }
    bool isValid() const { return m_comp && m_it != m_comp->end(); }
    void *creator() const { return m_creator; }
    const QVector<QRectF> &boundingBoxes() const { return m_bboxes; }
    int gap() const { return m_gap; }
private:
    friend class SubtitleDrawer;
    const SubComp *m_comp = nullptr;
    SubComp::const_iterator m_it; RichTextDocument m_text;
    QVector<QRectF> m_bboxes;
    int m_gap = 0;
    void *m_creator = nullptr;
};

class SubtitleDrawer {
public:
    void setStyle(const SubtitleStyle &style);
    void setAlignment(Qt::Alignment alignment) {
        m_back.setAlignment(m_alignment = alignment);
        m_front.setAlignment(alignment);
    }
    void setMargin(const Margin &margin) { m_margin = margin; }
    bool hasDrawn() const {return m_drawn;}
    QVector<QRectF> draw(QImage &image, int &gap, const RichTextDocument &text, const QRectF &area, double dpr = 1.0);
    bool draw(SubCompImage &pic, const QRectF &area, double dpr = 1.0) {
        pic.m_bboxes = draw(pic, pic.m_gap, pic.m_text, area, dpr);
        return !pic.isNull();
    }
    QPointF pos(const QSizeF &image, const QRectF &area) const;
    Qt::Alignment alignment() const { return m_alignment; }
    const Margin &margin() const { return m_margin; }
    const SubtitleStyle &style() const {return m_style;}
    double scale(const QRectF &area) const {
        const auto policy = m_style.font.scale;
        double px = m_style.font.size;
        if (policy == SubtitleStyle::Font::Scale::Diagonal)
            px *= _Diagonal(area.size());
        else if (policy == SubtitleStyle::Font::Scale::Width)
            px *= area.width();
        else
            px *= area.height();
        return px/m_style.font.height();
    }
private:
    static void updateStyle(RichTextDocument &doc, const SubtitleStyle &style) {
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
    SubtitleStyle m_style;
    RichTextDocument m_front, m_back;
    Margin m_margin;
    Qt::Alignment m_alignment;
    bool m_drawn = false;
    FastAlphaBlur m_blur;
    QByteArray m_buffer;
};

#endif // SUBTITLEDRAWER_HPP
