#include "subtitle_parser_p.hpp"
#include "misc/log.hpp"
#include <QTextStream>

DECLARE_LOG_CONTEXT(Subtitle)

int SubtitleParser::msPerChar = -1;

auto SubtitleParser::append(Subtitle &s, SubComp::SyncType b) -> SubComp&
{
    static int id = 0;
    s.m_comp.append(SubComp(type(), m_file, m_encoding, id++, b));
    return s.m_comp.last();
}

auto SubtitleParser::parse(const QString &fileName,
                           const EncodingInfo &enc) -> Subtitle
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly) || file.size() > (1 << 20))
        return Subtitle();
    QTextStream in;
    in.setDevice(&file);
    in.setCodec(enc.codec());
    const QString all = in.readAll();
    QFileInfo info(fileName);
    Subtitle sub;

    auto name = [] (SubType type) -> QString {
        switch (type) {
        case SubType::SAMI:
            return u"SAMI"_q;
        case SubType::SubRip:
            return u"SubRip"_q;
        case SubType::TMPlayer:
            return u"TMPlayer"_q;
        case SubType::MicroDVD:
            return u"MicroDVD"_q;
        default:
            return u"Unknown"_q;
        }
    };
    auto tryIt = [&] (SubtitleParser *p) {
        p->m_all = all;
        p->m_file = info;
        p->m_encoding = enc;
        const bool parsable = p->isParsable();
        _Info("Trying (parser: %%, encoding: %%, file: %%): %%",
               name(p->type()), enc.name(), file.fileName(), parsable);
        if (parsable)
            p->_parse(sub);
        delete p;
        return parsable;
    };

    if (tryIt(new SamiParser) || tryIt(new SubRipParser)
            || tryIt(new MicroDVDParser) || tryIt(new TMPlayerParser))
        return sub;
    return Subtitle();
}

auto SubtitleParser::processLine(int &idx, const QString &texts) -> QStringRef
{
    int from = idx;
    idx = texts.indexOf(QLatin1Char('\n'), from);
    if (idx < 0)
        idx = texts.indexOf(QLatin1Char('\r'), from);
    if (idx < 0) {
        idx = texts.size();
        return texts.midRef(from);
    } else {
        return texts.midRef(from, (idx++) - from);
    }
}

auto SubtitleParser::predictEndTime(int start, const QString &text) -> int
{
    if (msPerChar > 0)
        return start + text.size()*msPerChar;
    return -1;
}

auto SubtitleParser::predictEndTime(int start, const QStringRef &text) -> int
{
    if (msPerChar > 0)
        return start + text.size()*msPerChar;
    return -1;
}

auto SubtitleParser::predictEndTime(const SubComp::const_iterator &it) -> int
{
    if (msPerChar > 0)
        return it.value().totalLength()*msPerChar + it.key();
    return -1;
}

auto SubtitleParser::encodeEntity(const QStringRef &str) -> QString
{
    QString ret;
    for (int i=0; i<str.size(); ++i) {
        ushort c = str.at(i).unicode();
        switch (c) {
        case ' ':
            ret += u"&nbsp;"_q;
            break;
        case '<':
            ret += u"&lt;"_q;
            break;
        case '>':
            ret += u"&gt;"_q;
            break;
        case '|':
            ret += u"<br>"_q;
            break;
        default:
            ret += str.at(i);
            break;
        }
    }
    return ret;
}

auto SubtitleParser::getLine() const -> QStringRef
{
    int from = m_pos;
    int end = -1;
    while (m_pos < m_all.size()) {
        if (at(m_pos) == '\r') {
            end = m_pos;
            ++m_pos;
            if (m_pos < m_all.size() && at(m_pos) == '\n')
                ++m_pos;
            break;
        } else if (at(m_pos) == '\n') {
            end = m_pos;
            ++m_pos;
            break;
        }
        ++m_pos;
    }
    if (end < 0)
        end = m_all.size();
    return from < m_all.size() ? m_all.midRef(from, end - from) : QStringRef();
}

