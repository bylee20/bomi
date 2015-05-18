#include "subtitle_parser_p.hpp"

SCIA _TimeToMSec(int h, int m, int s, int ms = 0) -> qint64
{ return ((h * 60 + m) * 60 + s) * 1000 + ms; }


auto SamiParser::isParsable() const -> bool
{
    const QRegEx rx(uR"(<\s*(sami|body|sync))"_q, QRegEx::CaseInsensitiveOption);
    return all().contains(rx);
}

auto SamiParser::_parse(Subtitle &sub) -> void
{
    const QString &text = this->all();
    sub.clear();
    int pos = 0;
    while (pos < text.size()) {
        const QChar c = text.at(pos);
        if (c.unicode() != '<') {
            ++pos;
            continue;
        }
        Tag tag = parseTag(text, pos);
        if (_Same(tag.name, "body"))
            break;
        if (_Same(tag.name, "sync")) {
            pos = tag.pos;
            break;
        }
    }
    RichTextBlockParser parser(text.midRef(pos));
    auto &comps = components(sub);
    while (!parser.atEnd()) {
        Tag tag;
        const auto block_sync = parser.get(u"sync"_q, u"/?sync|/body|/sami"_q,
                                           &tag);
        if (tag.name.isEmpty())
            break;
        const int sync = toInt(tag.value("start"));
        QMap<QString, QList<RichTextBlock> > blocks;
        RichTextBlockParser p(block_sync);
        while (!p.atEnd()) {
            const QList<RichTextBlock> paragraph = p.paragraph(&tag);
            blocks[tag.value("class").toString()] += paragraph;
        }
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            SubComp *comp = nullptr;
            for (int i=0; i<sub.count(); ++i) {
                if (comps[i].language() == it.key()) {
                    comp = &comps[i];
                    break;
                }
            }
            if (!comp) {
                comp = &append(sub);
                comp->setLanguage(it.key());
            }
            (*comp)[sync] += it.value();
        }
    }
}



auto SubRipParser::isParsable() const -> bool
{
    return all().contains(rx);
}

auto SubRipParser::_parse(Subtitle &sub) -> void
{
    sub.clear();
    auto &comp = append(sub);
    const auto &all = this->all();
    auto prev = rx.match(all);
    if (!prev.hasMatch())
        return;

    for(;;) {
        const auto m = rx.match(all, prev.capturedEnd());
        const int begin = prev.capturedEnd();
        const int end = m.hasMatch() ? m.capturedStart() : all.size();
        auto caption = all.midRef(begin, end - begin).trimmed().toString();
        caption.replace(QRegEx(uR"((\r\n|\n|\r|\\N))"_q), u"<br>"_q);
        caption.replace("\\h"_a, u"&nbsp;"_q);
        if (caption.isEmpty())
            caption = u"<br>"_q;
#define TO_INT(n) (prev.capturedRef(n).toInt())
        const int t1 = _TimeToMSec(TO_INT(3), TO_INT(4), TO_INT(5), TO_INT(6));
        const int t2 = _TimeToMSec(TO_INT(7), TO_INT(8), TO_INT(9), TO_INT(10));
#undef TO_INT
        append(comp, "<p>"_a % caption % "</p>"_a, t1, t2);
        if (!m.hasMatch())
            break;
        prev = m;
    }
}

/******************************************************************************/

auto LineParser::isParsable() const -> bool
{
    if (skipSeparators())
        return false;
    const int pos = this->pos();
    auto match = this->match(trim(getLine()).toString());
    if (!match.hasMatch())
        return false;
    seekTo(pos);
    return true;
}

auto TMPlayerParser::_parse(Subtitle &sub) -> void
{
    sub.clear();
    auto &comp = append(sub);
    int predictedEnd = -1;
    while (!atEnd()) {
        auto m = match(getLine().toString());
        if (!m.hasMatch())
            continue;
        auto toInt = [&] (int nth) { return m.capturedRef(nth).toInt(); };
        const int time = _TimeToMSec(toInt(1), toInt(2), toInt(3));
        if (predictedEnd > 0 && time > predictedEnd)
            comp[predictedEnd];
        auto text = m.capturedRef(4);
        predictedEnd = predictEndTime(time, text);
        append(comp, "<p>"_a % encodeEntity(trim(text)) % "</p>"_a, time);
    }
}

auto MicroDVDParser::_parse(Subtitle &sub) -> void
{
    QRegExMatch m;
    while (!atEnd()) {
        m = match(trim(getLine()).toString());
        if (m.hasMatch())
            break;
    }
    if (!m.hasMatch())
        return;
    bool ok = false;
    const double fps = m.capturedRef(3).toDouble(&ok);
    auto getKey = [ok, fps] (int frame)
        { return ok ? qRound((frame/fps)*1000.0) : frame; };
    seekTo(0);
    append(sub, ok ? SubComp::Time : SubComp::Frame);

    QRegEx rxAttr(uR"(\{([^\}]+):([^\}]+)\})"_q);
    SubComp &comp = components(sub).front();
    while (!atEnd()) {
        m = match(trim(getLine()).toString());
        if (!m.hasMatch())
            continue;
        const int start = getKey(m.capturedRef(1).toInt());
        const int end = getKey(m.capturedRef(2).toInt());
        const auto text = m.captured(3);
        QString parsed1, parsed2;
        auto addTag0 = [&] (const QString &name) {
            parsed1 += '<'_q % name % '>'_q;
            parsed2 += "</"_a % name % '>'_q;
        };
        auto addTag1 = [&] (const QString &name, const QString &attr) {
            parsed1 += '<'_q % name % ' '_q % attr % '>'_q;
            parsed2 += "</"_a % name % '>'_q;
        };
        int idx = 0;
        QRegEx rxColor(u"\\$([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})"_q);
        QRegExMatch am;
        while ((am = rxAttr.match(text, idx)).hasMatch()) {
            const auto name = am.capturedRef(1);
            const auto value = am.capturedRef(2);
            if (_Same(name, "y")) {
                if (value.contains('i'_q, QCI))
                    addTag0(u"i"_q);
                if (value.contains('u'_q, QCI))
                    addTag0(u"u"_q);
                if (value.contains('s'_q, QCI))
                    addTag0(u"s"_q);
                if (value.contains('b'_q, QCI))
                    addTag0(u"b"_q);
            } else if (_Same(name, "c")) {
                auto cm = rxColor.match(value.toString());
                if (cm.hasMatch())
                    addTag1(u"font"_q,
                            "color=\"#"_a % cm.capturedRef(3)
                            % cm.capturedRef(2) % cm.capturedRef(1) % '"'_q);
            }
            idx = am.capturedEnd();
        }
        QString caption;
        if (idx < text.size()) {
            if (text[idx] == '/'_q) {
                addTag0(u"i"_q);
                ++idx;
            }
            caption = "<p>"_a % parsed1
                      % replace(text.midRef(idx), u"|"_q, u"<br>"_q)
                      % parsed2 % "</p>"_a;
        } else
            caption = "<p>"_a % parsed1 % parsed2 % "</p>"_a;
        append(comp, caption, start, end);
    }
}
