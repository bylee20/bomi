#include "richtextblock.hpp"

RichTextBlockParser::RichTextBlockParser(const QStringRef &text)
    : m_text(text)
    , m_pos(0)
    , m_good(1) { }

auto RichTextBlockParser::get(const char *open, const char *close,
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
    QStringRef paragraph = trim(get("p", "/?sync|/?p|/body|/sami", tag));
    if (m_good)
        return parse(paragraph, tag ? tag->style() : RichTextBlock::Style());
    return QList<RichTextBlock>();
}

auto RichTextBlockParser::parse(const QStringRef &text,
                                const Style &style) -> QList<RichTextBlock>
{
    QList<RichTextBlock> ret;

    auto add_format = [&ret] (const RichTextBlock::Style &style) {
        ret.last().formats.append(RichTextBlock::Format());
        ret.last().formats.last().begin = ret.last().text.size();
        ret.last().formats.last().end = -1;
        ret.last().formats.last().style = style;
    };

    auto add_block = [&ret, &add_format] (bool paragraph, const RichTextBlock::Style &style) {
        ret.append(RichTextBlock(paragraph));
        add_format(style);
    };

    QLinkedList<RichTextBlock::Style> fmtStack;
    QLinkedList<QStringRef> tagStack;
    add_block(true, style);

    int pos = 0;

    auto add_text = [&ret] (const QStringRef &text) {
        int pos = 0;
        QString seps;
        while (pos < text.size()) {
            const QChar c = text.at(pos);
            if (isSeparator(c.unicode())) {
                seps += _L(' ');
                if (skipSeparator(pos, text))
                    break;
            } else {
                if (!seps.isEmpty()) {
                    if (!ret.last().text.isEmpty())
                        ret.last().text.append(_L(' '));
                    seps.clear();
                }
                if (c.unicode() == '&') {
                    const int idx = text.indexOf(';', ++pos);
                    if (idx < 0)
                        ret.last().text.append(c);
                    else {
                        ret.last().text.append(entityCharacter(_MidRef(text, pos, idx - pos)));
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
        const int idx = text.indexOf('<', pos);
        if (idx < 0) {
            add_text(_MidRef(text, pos, -1));
            break;
        } else {
            add_text(_MidRef(text, pos, idx - pos));
            pos = idx;
        }
        Tag tag = parseTag(_MidRef(text, 0, -1), pos);
        if (tag.name.isEmpty())
            continue;
        ret.last().formats.last().end = ret.last().text.size();

        if (_Same(tag.name, "br") || _Same(tag.name, "/br")) { // new block
            add_block(false, ret.last().formats.last().style);
        } else {
            if (tag.name.startsWith('/')) { // restore format
                auto fmtIt = fmtStack.begin();
                auto tagIt = tagStack.begin();
                for (; tagIt != tagStack.end(); ++tagIt, ++fmtIt) {
                    if (tagIt->compare(_MidRef(tag.name, 1), Qt::CaseInsensitive) == 0) {
                        add_format(*fmtIt);
                        fmtStack.erase(fmtIt);
                        tagStack.erase(tagIt);
                        break;
                    }
                }
                if (_Same(tag.name, "/ruby")) {
                    if (ruby) {
//                        const int idx = indexOf(text, rx, pos);
//                        if (idx < 0)
//                            continue;
//                        add_format(ret.last().formats.last().style);
//                        ret.last().rubies.push_back(RichTextBlock::Ruby());
//                        RichTextBlock::Ruby &ruby = ret.last().rubies.last();
//                        RichTextBlock::Format &rb_format = ret.last().formats.last();
//                        rb_format.mergeStyle(tag.style());

//                        ruby.rb_begin = rb_format.begin = ret.last().text.size();
//                        add_text(rx.cap(2).midRef(0, -1));
//                        ruby.rb_end = rb_format.end = ret.last().text.size();
//                        ruby.rb_style = rb_format.style;
//                        ruby.rt = rx.cap(4);

//                        add_format(ret.last().formats[ret.last().formats.size()-2].style);

//                        pos = idx + rx.matchedLength();
                        if (ruby->rb_end < 0)
                            ruby->rb_end = ret.last().text.size();
                    }
                    ruby = nullptr;
                }
            } else { // new format
                if (_Same(tag.name, "rp")) {
                    QRegularExpression regex(R"((.*)(<\s*/rp\s*>|<\s*rt\s*>|<\s*/ruby\s*>|$))", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
                    const auto match = regex.match(text.toString(), pos);
                    pos = match.capturedEnd();
                    if (ruby && ruby->rb_end < 0)
                        ruby->rb_end = ret.last().text.size();
                } else if (_Same(tag.name, "rt")) {
                    if (!ruby)
                        continue;
                    QRegularExpression regex(R"((.*)(<\s*/rt\s*>|<\s*/ruby\s*>|$))", QRegularExpression::InvertedGreedinessOption | QRegularExpression::CaseInsensitiveOption);
                    const auto match = regex.match(text.toString(), pos);
                    Q_ASSERT(match.hasMatch());
                    ruby->rt_block = parse(match.capturedRef(1), ret.last().formats.last().style).value(0);
                    qDebug() << ruby->rt_block.text;
                    if (ruby->rb_end < 0)
                        ruby->rb_end = ret.last().text.size();
                    pos = match.capturedEnd();
//                    qDebug() <<match.captured(1);
//                    match.
//                    if (!match.hasMatch())
//                        continue;

//                    QRegExp rx("(.*)(<\\s*rb\\s*>)?([^<]*)(<\\s*/rb\\s*>)?<\\s*rt\\s*>([^<]*)(<\\s*/rt\\s*>)?(<\\s*/ruby\\s*>|$)");
//                    const int idx = indexOf(text, rx, pos);
//                    if (idx < 0)
//                        continue;
//                    add_format(ret.last().formats.last().style);
//                    ret.last().rubies.push_back(RichTextBlock::Ruby());
//                    RichTextBlock::Ruby &ruby = ret.last().rubies.last();
//                    RichTextBlock::Format &rb_format = ret.last().formats.last();
//                    rb_format.mergeStyle(tag.style());

//                    ruby.rb_begin = rb_format.begin = ret.last().text.size();
//                    add_text(rx.cap(2).midRef(0, -1));
//                    ruby.rb_end = rb_format.end = ret.last().text.size();
//                    ruby.rb_style = rb_format.style;
//                    ruby.rt = rx.cap(4);

//                    add_format(ret.last().formats[ret.last().formats.size()-2].style);

//                    pos = idx + rx.matchedLength();
//                    pos =
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


//                    if (_Same(tag.name, "ruby")) {//((<\s*rt\s*>(.*)(<\s*/rt\s*>)?)?)(.*)
////                        QRegularExpression outer(R"((.*)<\s*rt\s*>(.*)(<\s*/ruby\s*>|<\s*/p\s*>|$))", QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);

//                        QRegExp rx("(<\\s*rb\\s*>)?([^<]*)(<\\s*/rb\\s*>)?<\\s*rt\\s*>([^<]*)(<\\s*/rt\\s*>)?(<\\s*/ruby\\s*>|$)");
//                        const int idx = indexOf(text, rx, pos);
//                        if (idx < 0)
//                            continue;
//                        add_format(ret.last().formats.last().style);
//                        ret.last().rubies.push_back(RichTextBlock::Ruby());
//                        RichTextBlock::Ruby &ruby = ret.last().rubies.last();
//                        RichTextBlock::Format &rb_format = ret.last().formats.last();
//                        rb_format.mergeStyle(tag.style());

//                        ruby.rb_begin = rb_format.begin = ret.last().text.size();
//                        add_text(rx.cap(2).midRef(0, -1));
//                        ruby.rb_end = rb_format.end = ret.last().text.size();
//                        ruby.rb_style = rb_format.style;
//                        ruby.rt = rx.cap(4);

//                        add_format(ret.last().formats[ret.last().formats.size()-2].style);

//                        pos = idx + rx.matchedLength();
//                    } else {
//                        fmtStack.push_front(ret.last().formats.last().style);
//                        tagStack.push_front(tag.name);
//                        add_format(ret.last().formats.last().style);
//                        ret.last().formats.last().mergeStyle(tag.style());
//                    }
//                }
            }
        }
    }

    if (ret.last().formats.last().end < 0)
        ret.last().formats.last().end = ret.last().text.size();

    return ret;
}
