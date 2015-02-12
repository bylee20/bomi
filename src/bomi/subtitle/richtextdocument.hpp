#ifndef RICHTEXTDOCUMENT_HPP
#define RICHTEXTDOCUMENT_HPP

#include "richtextblock.hpp"
#include "richtexthelper.hpp"
#include <QTextLayout>

class RichTextDocument : public RichTextHelper {
public:
    RichTextDocument();
    RichTextDocument(const QString &text);
    RichTextDocument(const RichTextDocument &rhs);
    ~RichTextDocument();
    RichTextDocument &operator = (const RichTextDocument &rhs);
    RichTextDocument &operator = (const QList<RichTextBlock> &rhs);
    RichTextDocument &operator += (const RichTextDocument &rhs);
    RichTextDocument &operator += (const QList<RichTextBlock> &rhs);
    inline auto isEmpty() const -> bool {return m_blocks.isEmpty();}
    inline auto totalLength() const -> int {
        int ret = 0;
        for (auto &block : m_blocks)
            ret += block.text.size();
        return ret;
    }
    inline auto hasWords() const -> bool {
        for (auto &block : m_blocks) {
            if (block.hasWords())
                return true;
        }
        return false;
    }
    inline auto toPlainText() const -> QString {
        QString ret;
        for (auto &block : m_blocks)
            ret += block.text;
        return ret;
    }
    inline const QList<RichTextBlock> &blocks() const {return m_blocks;}
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setWrapMode(QTextOption::WrapMode wrapMode) -> void;
    auto setFormat(QTextFormat::Property property, const QVariant &data) -> void;
    auto draw(QPainter *painter, const QPointF &pos) -> void;
    auto drawBoudingBoxes(QPainter *painter, const QPointF &pos) -> void;
    auto doLayout(double maxWidth) -> void;
    auto updateLayoutInfo() -> void;
    auto setText(const QString &text) -> void;
    auto setFontPixelSize(int px) -> void;
    auto setTextOutline(const QColor &color, double width) -> void {setTextOutline(QPen(color, width));}
    auto setTextOutline(const QPen &pen) -> void;
    auto naturalSize() const -> QSizeF {return m_natural.size();}
    auto setLeading(double newLine, double paragraph) -> void;
    auto clear() -> void { freeLayouts(); m_blocks.clear(); setChanged(true); }
    const QVector<QRectF> &boundingBoxes() const { return m_boxes; }
private:
    struct Layout {
        QTextLayout block;
        QVector<QTextLayout*> rubies;
    };
    auto freeLayouts() -> void;
    inline auto setChanged(bool changed) -> void {
        m_dirty = m_blockChanged = m_formatChanged = m_pxChanged = m_optionChanged = changed;
    }
    QList<RichTextBlock> m_blocks;
    QVector<QRectF> m_boxes;
    QTextOption m_option;
    QTextCharFormat m_format;
    double m_lineLeading = 0, m_paragraphLeading = 0;

    QVector<Layout*> m_layouts;
    bool m_blockChanged, m_formatChanged, m_optionChanged, m_pxChanged, m_dirty;
    QRectF m_natural;
};

#endif // RICHTEXTDOCUMENT_HPP
