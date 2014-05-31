#include "subtitle_parser_p.hpp"

SCIA _TimeToMSec(int h, int m, int s, int ms = 0) -> qint64
{ return ((h * 60 + m) * 60 + s) * 1000 + ms; }


auto SamiParser::isParsable() const -> bool
{
    if (_Same(file().suffix(), "smi") || _Same(file().suffix(), "sami"))
        return true;
    if (skipSeparators())
        return false;
    if (all().startsWith("<sami", QCI))
        return true;
    return false;
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
        const auto block_sync = parser.get("sync", "/?sync|/body|/sami", &tag);
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
    if (_Same(file().suffix(), "srt"))
        return true;
    return false;
}

auto SubRipParser::_parse(Subtitle &sub) -> void
{
    QRegEx rxNum(R"(^\s*(\d+)\s*$)");
    QRegEx rxTime(R"(^\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*)"
                              R"(-->\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*$)");
    QRegEx rxBlank(R"(^\s*$)");
    auto getNumber = [&rxNum, this] () {
        for (;;) {
            const auto ref = getLine();
            if (ref.isNull())
                break;
            auto matched = rxNum.match(ref.toString());
            if (matched.hasMatch())
                return matched.capturedRef(1).toInt();
        }
        return -1;
    };
    auto getTime = [&rxTime, this] (int &start, int &end) {
        for (;;) {
            const auto ref = getLine();
            if (ref.isNull())
                break;
            auto matched = rxTime.match(ref.toString());
            if (matched.hasMatch()) {
#define TO_INT(n) (matched.capturedRef(n).toInt())
                start = _TimeToMSec(TO_INT(1), TO_INT(2), TO_INT(3), TO_INT(4));
                end = _TimeToMSec(TO_INT(5), TO_INT(6), TO_INT(7), TO_INT(8));
#undef TO_INT
                return true;
            }
        }
        return false;
    };
    auto getCaption = [&rxBlank, this] () {
        QString ret;
        for (;;) {
            const auto line = getLine().toString();
            auto matched = rxBlank.match(line);
            if (matched.hasMatch())
                break;
            if (!ret.isEmpty())
                ret += "<br>";
            ret += line;
        }
        return QString("<p>" % ret % "</p>");
    };

    sub.clear();
    auto &comp = append(sub);
    for (;;) {
        const auto num = getNumber();
        if (num < 0)
            break;
        int start = 0, end = 0;
        if (!getTime(start, end))
            break;
        const auto caption = getCaption();
        append(comp, caption, start, end);
    }
}

auto TMPlayerParser::_parse(Subtitle &sub) -> void
{
    sub.clear();
    auto &comp = append(sub);
    int predictedEnd = -1;
    auto toInt = [this] (int nth) {return rxLine.cap(nth).toInt();};
    while (!atEnd()) {
        auto line = getLine().toString();
        if (line.indexOf(rxLine) == -1)
            continue;
        const int time = _TimeToMSec(toInt(1), toInt(2), toInt(3));
        if (predictedEnd > 0 && time > predictedEnd)
            comp[predictedEnd];
        QString text = rxLine.cap(4);
        predictedEnd = predictEndTime(time, text);
        text = "<p>"_a % encodeEntity(trim(text.midRef(0))) % "</p>"_a;
        append(comp, text, time);
    }
}

auto MicroDVDParser::_parse(Subtitle &sub) -> void
{
    QString line;
    int begin = -1;
    while (!atEnd() && begin == -1) {
        line = trim(getLine()).toString();
        begin = rxLine.indexIn(line);
    }
    if (begin == -1)
        return;
    bool ok = false;
    const double fps = rxLine.cap(3).toDouble(&ok);
    auto getKey = [ok, fps] (int frame)
        {return ok ? qRound((frame/fps)*1000.0) : frame;};
    if (ok) {
        append(sub, SubComp::Time);
    } else {
        seekTo(0);
        append(sub, SubComp::Frame);
    }

    QRegExp rxAttr("\\{([^\\}]+):([^\\}]+)\\}");
    SubComp &comp = components(sub).first();
    while (!atEnd()) {
        line = trim(getLine()).toString();
        if (rxLine.indexIn(line) == -1)
            continue;
        const int start = getKey(rxLine.cap(1).toInt());
        const int end = getKey(rxLine.cap(2).toInt());
        QString text = rxLine.cap(3);
        QString parsed1, parsed2;
        auto addTag0 = [&] (const QString &name) {
            parsed1 += '<' % name % '>';
            parsed2 += "</"_a % name % '>';
        };
        auto addTag1 = [&] (const QString &name, const QString &attr) {
            parsed1 += '<' % name % ' ' % attr % '>';
            parsed2 += "</"_a % name % '>';
        };
        int idx = 0;
        QRegExp rxColor("\\$([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");
        while (text.indexOf(rxAttr, idx) != -1) {
            const auto name = rxAttr.cap(1);
            const auto value = rxAttr.cap(2);
            if (_Same(name, "y")) {
                if (value.contains('i', QCI))
                    addTag0("i");
                if (value.contains('u', QCI))
                    addTag0("u");
                if (value.contains('s', QCI))
                    addTag0("s");
                if (value.contains('b', QCI))
                    addTag0("b");
            } else if (_Same(name, "c")) {
                if (rxColor.indexIn(value) != -1)
                    addTag1(u"font"_q, _L("color=\"#") % rxColor.cap(3)
                            % rxColor.cap(2) % rxColor.cap(1) %_L("\""));
            }
            idx = rxAttr.pos() + rxAttr.matchedLength();
        }
        if (idx < text.size()) {
            if (text[idx] == '/') {
                addTag0("i");
                ++idx;
            }
            text = "<p>"_a % parsed1 % replace(text.midRef(idx), u"|"_q, u"<br>"_q)
                    % parsed2 % "</p>"_a;
        } else
            text = "<p>"_a % parsed1 % parsed2 % "</p>"_a;
        append(comp, text, start, end);
    }
}
