#include "richtextblock.hpp"

RichTextBlockParser::RichTextBlockParser(const QStringRef &text)
    : m_text(text)
    , m_pos(0)
    , m_good(1) { }

auto RichTextBlockParser::get(const QString &open, const QString &close,
                              Tag *tag) -> QStringRef
{
    Tag _tag;
    if (!tag)
        tag = &_tag;
    QStringRef ret;
    m_good = innerText(open, close, m_text, ret, m_pos, *tag);
    if (m_good <= 0)
        m_pos = m_text.size();
    return ret;
}

auto RichTextBlockParser::paragraph(Tag *tag) -> QList<RichTextBlock>
{
    if (m_pos >= m_text.size())
        return QList<RichTextBlock>();
    QStringRef paragraph = trim(get(u"p"_q, u"/?sync|/?p|/body|/sami"_q, tag));
    if (m_good)
        return parse(paragraph, tag ? tag->style() : RichTextBlock::Style());
    return QList<RichTextBlock>();
}

auto RichTextBlockParser::parse(const QStringRef &text,
                                const Style &style) -> QList<RichTextBlock>
{
    QList<RichTextBlock> ret;

    auto add_format = [&ret] (RichTextBlock::Style style) {
        ret.last().formats.append(RichTextBlock::Format());
        ret.last().formats.last().begin = ret.last().text.size();
        ret.last().formats.last().end = -1;
        ret.last().formats.last().style = style;
    };

    auto add_block = [&ret, &add_format] (bool paragraph,
                                          RichTextBlock::Style style) {
        ret.append(RichTextBlock(paragraph));
        add_format(style);
    };

    std::list<RichTextBlock::Style> fmtStack;
    std::list<QStringRef> tagStack;
    add_block(true, style);

    int pos = 0;

    auto add_text = [&ret] (const QStringRef &text) {
        int pos = 0;
        QString seps;
        while (pos < text.size()) {
            const QChar c = text.at(pos);
            if (isSeparator(c.unicode())) {
                seps += ' '_q;
                if (skipSeparator(pos, text))
                    break;
            } else {
                if (!seps.isEmpty()) {
                    if (!ret.last().text.isEmpty())
                        ret.last().text.append(' '_q);
                    seps.clear();
                }
                if (c.unicode() == '&') {
                    const int idx = text.indexOf(';'_q, ++pos);
                    if (idx < 0)
                        ret.last().text.append(c);
                    else {
                        const auto ref = text.mid(pos, idx - pos);
                        ret.last().text.append(entityCharacter(ref));
                        pos = idx + 1;
                    }
                } else {
                    ret.last().text.append(c);
                    ++pos;
                }
            }
        }
    };

    RichTextBlock::Ruby *ruby = nullptr;

    while (pos <text.size()) {
        const int idx = text.indexOf('<'_q, pos);
        if (idx < 0) {
            add_text(text.mid(pos, -1));
            break;
        } else {
            add_text(text.mid(pos, idx - pos));
            pos = idx;
        }
        Tag tag = parseTag(text.mid(0, -1), pos);
        if (tag.name.isEmpty())
            continue;
        ret.last().formats.last().end = ret.last().text.size();

        if (_Same(tag.name, "br") || _Same(tag.name, "/br")) { // new block
            add_block(false, ret.last().formats.last().style);
        } else {
            if (tag.name.startsWith('/'_q)) { // restore format
                auto fmtIt = fmtStack.begin();
                auto tagIt = tagStack.begin();
                for (; tagIt != tagStack.end(); ++tagIt, ++fmtIt) {
                    if (tagIt->compare(tag.name.mid(1), QCI) == 0) {
                        add_format(*fmtIt);
                        fmtStack.erase(fmtIt);
                        tagStack.erase(tagIt);
                        break;
                    }
                }
                if (_Same(tag.name, "/ruby")) {
                    if (ruby) {
                        if (ruby->rb_end < 0)
                            ruby->rb_end = ret.last().text.size();
                    }
                    ruby = nullptr;
                }
            } else { // new format
                constexpr auto options = QRegEx::InvertedGreedinessOption
                                       | QRegEx::CaseInsensitiveOption;
                if (_Same(tag.name, "rp")) {
                    QRegEx regex(uR"((.*)(<\s*/rp\s*>|<\s*rt\s*>)"
                                 uR"(|<\s*/ruby\s*>|$))"_q, options);
                    const auto match = regex.match(text.toString(), pos);
                    pos = match.capturedEnd();
                    if (ruby && ruby->rb_end < 0)
                        ruby->rb_end = ret.last().text.size();
                } else if (_Same(tag.name, "rt")) {
                    if (!ruby)
                        continue;
                    QRegEx regex(uR"((.*)(<\s*/rt\s*>|<\s*/ruby\s*>)"
                                 uR"(|$))"_q, options);
                    const auto match = regex.match(text.toString(), pos);
                    Q_ASSERT(match.hasMatch());
                    const auto &st = ret.last().formats.last().style;
                    ruby->rt_block = parse(match.capturedRef(1), st).value(0);
                    if (ruby->rb_end < 0)
                        ruby->rb_end = ret.last().text.size();
                    pos = match.capturedEnd();
                } else {
                    if (_Same(tag.name, "ruby")) {
                        ret.last().rubies.push_back(RichTextBlock::Ruby());
                        ruby = &ret.last().rubies.last();
                        ruby->rb_begin = ret.last().text.size();
                    }
                    fmtStack.push_front(ret.last().formats.last().style);
                    tagStack.push_front(tag.name);
                    add_format(ret.last().formats.last().style);
                    ret.last().formats.last().mergeStyle(tag.style());
                }
            }
        }
    }

    if (ret.last().formats.last().end < 0)
        ret.last().formats.last().end = ret.last().text.size();

    return ret;
}
