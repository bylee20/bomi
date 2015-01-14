#ifndef RICHTEXTBLOCK_HPP
#define RICHTEXTBLOCK_HPP

#include "richtexthelper.hpp"

struct RichTextBlock {
    RichTextBlock(bool paragraph = true)
        : paragraph(paragraph) {}
    enum Property {
        ForegroundBrush = QTextFormat::ForegroundBrush,
        FontPixelSize = QTextFormat::FontPixelSize,
        FontWeight = QTextFormat::FontWeight,
        FontFamily = QTextFormat::FontFamily,
        FontItalic = QTextFormat::FontItalic,
        FontUnderline = QTextFormat::FontUnderline,
        FontStrikeOut = QTextFormat::FontStrikeOut
    };
    using Style = QMap<int, QVariant>;
    struct Format {
        auto mergeStyle(const Style &style) -> void
        {
            for (auto it = style.begin(); it != style.end(); ++it)
                this->style[it.key()] = it.value();
        }
        Style style;
        int begin, end;
    };
    struct Ruby;
    auto hasWords() const -> bool
    {
        for (auto &c : text) {
            if (!RichTextHelper::isSeparator(c.unicode()))
                return true;
        }
        return false;
    }
    QVector<Format> formats;
    QString text;
    bool paragraph;
    QVector<Ruby> rubies;
};

struct RichTextBlock::Ruby {
    int rb_begin = -1, rb_end = -1;
    RichTextBlock rt_block;
};

class RichTextBlockParser : public RichTextHelper {
public:
    RichTextBlockParser(const QStringRef &text);
    auto atEnd() const -> bool {return m_pos >= m_text.size();}
    auto get(const QString &open, const QString &close,
             Tag *tag = nullptr) -> QStringRef;
    auto paragraph(Tag *tag = nullptr) -> QList<RichTextBlock>;
protected:
    using Style = RichTextBlock::Style;
    static auto parse(const QStringRef &text,
                      const Style &style = Style()) -> QList<RichTextBlock>;
    QStringRef m_text;
    int m_pos, m_good;
};

#endif // RICHTEXTBLOCK_HPP
