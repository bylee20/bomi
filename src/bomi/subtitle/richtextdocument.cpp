#include "richtextdocument.hpp"

RichTextDocument::RichTextDocument()
{
    setChanged(false);
}

RichTextDocument::RichTextDocument(const QString &text)
{
    setText(text);
}

RichTextDocument::RichTextDocument(const RichTextDocument &rhs)
    : m_blocks(rhs.blocks())
    , m_option(rhs.m_option)
    , m_format(rhs.m_format)
    , m_lineLeading(rhs.m_lineLeading)
    , m_paragraphLeading(rhs.m_paragraphLeading)
{
    setChanged(!m_blocks.isEmpty());
}

RichTextDocument::~RichTextDocument()
{
    freeLayouts();
}

auto RichTextDocument::freeLayouts() -> void
{
    for (auto &layout : m_layouts) {
        qDeleteAll(layout->rubies);
        _Delete(layout);
    }
    m_layouts.clear();
}

auto RichTextDocument::operator = (const RichTextDocument &rhs)
-> RichTextDocument& {
    if (this != &rhs) {
        m_blocks = rhs.blocks();
        m_option = rhs.m_option;
        m_format = rhs.m_format;
        m_lineLeading = rhs.m_lineLeading;
        m_paragraphLeading = rhs.m_paragraphLeading;
        setChanged(true);
    }
    return *this;
}

auto RichTextDocument::operator = (const QList<RichTextBlock> &rhs)
-> RichTextDocument& {
    if (&m_blocks != &rhs) {
        m_blocks = rhs;
        setChanged(true);
    }
    return *this;
}

auto RichTextDocument::operator += (const RichTextDocument &rhs)
-> RichTextDocument& {
    return operator += (rhs.blocks());
}

auto RichTextDocument::operator += (const QList<RichTextBlock> &rhs)
-> RichTextDocument& {
    if (!rhs.isEmpty()) {
        setChanged(true);
        m_blocks += rhs;
    }
    return *this;
}

auto RichTextDocument::setAlignment(Qt::Alignment alignment) -> void
{
    if (m_option.alignment() != alignment) {
        m_option.setAlignment(alignment);
        m_dirty = m_optionChanged = true;
    }
}

auto RichTextDocument::setWrapMode(QTextOption::WrapMode wrapMode) -> void
{
    if (m_option.wrapMode() != wrapMode) {
        m_option.setWrapMode(wrapMode);
        m_dirty = m_optionChanged = true;
    }
}

auto RichTextDocument::setFormat(QTextFormat::Property property,
                                 const QVariant &data) -> void
{
    if (m_format.property(property) != data) {
        m_format.setProperty(property, data);
        m_dirty = m_formatChanged = true;
    }
}

auto RichTextDocument::setLeading(double newLine, double paragraph) -> void
{
    if (!qFuzzyCompare(newLine, m_lineLeading)
            || !qFuzzyCompare(paragraph, m_paragraphLeading)) {
        m_lineLeading = newLine;
        m_paragraphLeading = paragraph;
        m_dirty = true;
    }
}

auto RichTextDocument::setFontPixelSize(int px) -> void
{
    if (m_format.intProperty(QTextFormat::FontPixelSize) != px) {
        m_format.setProperty(QTextFormat::FontPixelSize, px);
        m_dirty = m_pxChanged = true;
    }
}

auto RichTextDocument::setTextOutline(const QPen &pen) -> void
{
    if (m_format.penProperty(QTextFormat::TextOutline) != pen) {
        m_format.setProperty(QTextFormat::TextOutline, pen);
        m_dirty = m_pxChanged = true;
    }
}

auto RichTextDocument::setText(const QString &text) -> void
{
    m_blocks.clear();
    RichTextBlockParser parser(text.midRef(0));
    while (!parser.atEnd())
        m_blocks += parser.paragraph();
    setChanged(true);
}


static QTextOption makeRubyOption() {
    QTextOption opt;
    opt.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    return opt;
}

static const QTextOption rubyOption = makeRubyOption();

auto RichTextDocument::doLayout(double maxWidth) -> void
{
    if (!m_dirty)
        return;
    double width = -1;
    const int px = m_format.intProperty(QTextFormat::FontPixelSize);
    m_boxes.clear();
    QPointF pos(0, 0);
    for (int i=0; i<m_layouts.size(); ++i) {
        auto &block = m_layouts[i]->block;
        auto &rubies = m_layouts[i]->rubies;

        block.beginLayout();
        int rt_idx = 0;
        for (;;) {
            QTextLine line = block.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(maxWidth);
            line.setPosition(pos);
            double rt_height = 0.0;
            while (rt_idx < rubies.size()) {
                const auto &ruby = m_blocks[i].rubies[rt_idx];
                if (!(line.textStart() <= ruby.rb_begin
                      && ruby.rb_end <= line.textStart() + line.textLength()))
                    break;
                const int left = line.cursorToX(ruby.rb_begin);
                const int right = line.cursorToX(ruby.rb_end);
                if (left < right) {
                    QTextLayout *layout = rubies[rt_idx];
                    layout->beginLayout();
                    QTextLine line_rt = layout->createLine();
                    if (line_rt.isValid()) {
                        line_rt.setLineWidth(right - left);
                        line_rt.setPosition(QPointF(left, pos.y()));
                        if (rt_height < line_rt.height())
                            rt_height = line_rt.height();
                    }
                    layout->endLayout();
                }
                ++rt_idx;
            }

            const bool empty = !m_blocks[i].hasWords();
            if (!empty) {
                if (width < 0) {// first line
                    pos.ry() += rt_height;
                } else {
                    const auto leading = m_blocks[i].paragraph
                            ? m_paragraphLeading : m_lineLeading;
                    pos.ry() += qMax(rt_height, leading * px);
                }
                line.setPosition(pos);
                pos.ry() += line.height();
                if (line.naturalTextWidth() > width)
                    width = line.naturalTextWidth();
                m_boxes << line.naturalTextRect();
            } else if (width >= 0)
                pos.ry() += px;
        }
        block.endLayout();
    }
    m_natural = QRectF(0.0, 0.0, width, pos.ry());
    m_dirty = false;
}

auto RichTextDocument::updateLayoutInfo() -> void
{
    if (m_blockChanged) {
        freeLayouts();
        m_layouts.resize(m_blocks.size());
        m_formatChanged = m_optionChanged = true;
        for (int i=0; i<m_blocks.size(); ++i) {
            auto layout = m_layouts[i] = new Layout;
            layout->block.setText(m_blocks[i].text);
            layout->rubies.resize(m_blocks[i].rubies.size());
            for (int j=0; j<layout->rubies.size(); ++j) {
                layout->rubies[j] = new QTextLayout;
                layout->rubies[j]->setText(m_blocks[i].rubies[j].rt_block.text);
                layout->rubies[j]->setTextOption(rubyOption);
            }

        }
    }
    if (m_optionChanged) {
        for (auto layout : m_layouts)
            layout->block.setTextOption(m_option);
    }
    if (!m_formatChanged && m_pxChanged) {
        auto adjustFormats = [this] (QTextLayout *layout,
                                     const RichTextBlock &block, float r) {
            auto ranges = layout->additionalFormats();
            for (int j=0; j<ranges.size(); ++j) {
                const auto &style = block.formats[j].style;
                if (!style.contains(QTextFormat::FontPixelSize)) {
                    const auto size
                            = m_format.intProperty(QTextFormat::FontPixelSize);
                    const int px = qRound(size * r);
                    const auto ol = m_format.property(QTextFormat::TextOutline);
                    ranges[j].format.setProperty(QTextFormat::FontPixelSize, px);
                    ranges[j].format.setProperty(QTextFormat::TextOutline, ol);
                }
            }
            layout->setAdditionalFormats(ranges);
        };
        for (int i=0; i<m_blocks.size(); ++i) {
            const auto &block = m_blocks[i];
            auto layout = m_layouts[i];
            adjustFormats(&layout->block, block, 1);
            for (int j = 0; j < block.rubies.size(); ++j)
                adjustFormats(layout->rubies[j], block.rubies[j].rt_block, 0.45);
        }
    } else if (m_formatChanged) {
        auto mergeFormat = [this] (QTextCharFormat &format,
                                   const RichTextBlock::Style &style) {
            format = m_format;
            for (auto it = style.begin(); it != style.end(); ++it)
                format.setProperty(it.key(), it.value());
        };
        auto setFormats = [mergeFormat] (QTextLayout *layout,
                                         const RichTextBlock &blk, bool ruby) {
            QList<QTextLayout::FormatRange> ranges;
            for (auto &format : blk.formats) {
                ranges.push_back(QTextLayout::FormatRange());
                auto &range = ranges.last();
                range.start = format.begin;
                range.length = format.end - format.begin;
                mergeFormat(range.format, format.style);
                if (ruby) {
                    auto s = range.format.intProperty(QTextFormat::FontPixelSize);
                    const int px = s * 0.45 + 0.5;
                    range.format.setProperty(QTextFormat::FontPixelSize, px);
                }
            }
            layout->setAdditionalFormats(ranges);
        };

        for (int i=0; i<m_blocks.size(); ++i) {
            const auto &block = m_blocks[i];
            auto &layout = m_layouts[i];
            setFormats(&layout->block, block, false);
            for (int j = 0; j < block.rubies.size(); ++j)
                setFormats(layout->rubies[j], block.rubies[j].rt_block, true);
        }
    }
    m_blockChanged = m_formatChanged = m_pxChanged = m_optionChanged = false;
}

auto RichTextDocument::draw(QPainter *painter, const QPointF &pos) -> void
{
    for (auto layout : m_layouts) {
        layout->block.draw(painter, pos);
        for (auto ruby : layout->rubies)
            ruby->draw(painter, pos);
    }
}

auto RichTextDocument::drawBoudingBoxes(QPainter *painter,
                                        const QPointF &pos) -> void
{
    painter->save();
    painter->translate(pos);
    painter->setBrush(Qt::black);
    painter->drawRects(m_boxes);
    painter->restore();
}
