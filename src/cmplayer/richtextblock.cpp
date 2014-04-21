#include "richtextblock.hpp"


QStringRef RichTextBlockParser::get(const char *open, const char *close, Tag *tag) {
    Tag _tag;
    if (!tag)
        tag = &_tag;
    QStringRef ret;
    m_good = innerText(open, close, m_text, ret, m_pos, *tag);
    if (m_good <= 0)
        m_pos = m_text.size();
    return ret;
}

QList<RichTextBlock> RichTextBlockParser::paragraph(Tag *tag) {
    if (m_pos >= m_text.size())
        return QList<RichTextBlock>();
    QStringRef paragraph = trim(get("p", "/?sync|/?p|/body|/sami", tag));
    if (m_good)
        return parse(paragraph, tag ? tag->style() : RichTextBlock::Style());
    return QList<RichTextBlock>();
}

QList<RichTextBlock> RichTextBlockParser::parse(const QStringRef &text, const RichTextBlock::Style &style) {
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
            } else { // new format
                if (_Same(tag.name, "ruby")) {
                    QRegExp rx("(<\\s*rb\\s*>)?([^<]*)(<\\s*/rb\\s*>)?<\\s*rt\\s*>([^<]*)(<\\s*/rt\\s*>)?(<\\s*/ruby\\s*>|$)");
                    const int idx = indexOf(text, rx, pos);
                    if (idx < 0)
                        continue;
                    add_format(ret.last().formats.last().style);
                    ret.last().rubies.push_back(RichTextBlock::Ruby());
                    RichTextBlock::Ruby &ruby = ret.last().rubies.last();
                    RichTextBlock::Format &rb_format = ret.last().formats.last();
                    rb_format.mergeStyle(tag.style());

                    ruby.rb_begin = rb_format.begin = ret.last().text.size();
                    add_text(rx.cap(2).midRef(0, -1));
                    ruby.rb_end = rb_format.end = ret.last().text.size();
                    ruby.rb_style = rb_format.style;
                    ruby.rt = rx.cap(4);

                    add_format(ret.last().formats[ret.last().formats.size()-2].style);

                    pos = idx + rx.matchedLength();
                } else {
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
