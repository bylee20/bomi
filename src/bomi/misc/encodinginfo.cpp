#include "encodinginfo.hpp"
#include <QTextCodec>

EncodingInfo::EncodingInfo()
{

}

EncodingInfo::EncodingInfo(const QString &name, const QString &group,
                           const QString &sub)
    : m_name(name), m_group(group), m_subgroup(sub)
{

}

EncodingInfo::~EncodingInfo()
{

}

auto EncodingInfo::description() const -> QString
{
    if (m_subgroup.isEmpty())
        return m_group % ": "_a % m_name;
    return m_group % " ("_a % m_subgroup % "): "_a % m_name;
}

auto EncodingInfo::all() -> QVector<EncodingInfo>
{
    static const QVector<EncodingInfo> all = {
        { u"CP864"_q,           u"Arabic"_q },
        { u"ISO-8859-6"_q,      u"Arabic"_q },
        { u"Windows-1256"_q,    u"Arabic"_q },
        { u"CP775"_q,           u"Baltic"_q },
        { u"ISO-8859-4"_q,      u"Baltic"_q },
        { u"ISO-8859-13"_q,     u"Baltic"_q },
        { u"Windows-1257"_q,    u"Baltic"_q },
        { u"ISO-8859-14"_q,     u"Celtic"_q },
        { u"GBK"_q,             u"Chinese"_q, u"Simplified"_q },
        { u"HZ-GB-2312"_q,      u"Chinese"_q, u"Simplified"_q },
        { u"GB18030"_q,         u"Chinese"_q, u"Simplified"_q },
        { u"Big5"_q,            u"Chinese"_q, u"Traditional"_q },
        { u"Big5-HKSCS"_q,      u"Chinese"_q, u"Traditional"_q },
        { u"ISO-2022-CN"_q,     u"Chinese"_q, u"Traditional"_q },
        { u"ISO-2022-CN-EXT"_q, u"Chinese"_q, u"Traditional"_q },
        { u"CP855"_q,           u"Cyrillic"_q },
        { u"CP866"_q,           u"Cyrillic"_q },
        { u"ISO-8859-5"_q,      u"Cyrillic"_q },
        { u"KOI8-R"_q,          u"Cyrillic"_q },
        { u"KOI8-U"_q,          u"Cyrillic"_q },
        { u"Windows-1251"_q,    u"Cyrillic"_q },
        { u"CP852"_q,           u"European"_q, u"Central"_q },
        { u"ISO-8859-2"_q,      u"European"_q, u"Central"_q },
        { u"Windows-1250"_q,    u"European"_q, u"Central"_q },
        { u"CP861"_q,           u"European"_q, u"Icelandic"_q },
        { u"CP865"_q,           u"European"_q, u"Nordic"_q },
        { u"ISO-8859-10"_q,     u"European"_q, u"Nordic"_q },
        { u"ISO-8859-3"_q,      u"European"_q, u"South"_q },
        { u"CP858"_q,           u"European"_q, u"Western"_q },
        { u"CP850"_q,           u"European"_q, u"Western"_q },
        { u"ISO-8859-1"_q,      u"European"_q, u"Western"_q },
        { u"ISO-8859-15"_q,     u"European"_q, u"Western"_q },
        { u"Macintosh"_q,       u"European"_q, u"Western"_q },
        { u"Windows-1252"_q,    u"European"_q, u"Western"_q },
        { u"CP863"_q,           u"French"_q, u"Quebec"_q },
        { u"CP869"_q,           u"Greek"_q },
        { u"ISO-8859-7"_q,      u"Greek"_q },
        { u"Windows-1253"_q,    u"Greek"_q },
        { u"DOS-862"_q,         u"Hebrew"_q },
        { u"ISO-8859-8"_q,      u"Hebrew"_q },
        { u"ISO-8859-8"_q,      u"Hebrew"_q },
        { u"ISO-8859-8"_q,      u"Hebrew"_q },
        { u"Windows-1255"_q,    u"Hebrew"_q },
        { u"ISO-2022-JP-1"_q,   u"Japanese"_q },
        { u"ISO-2022-JP-2"_q,   u"Japanese"_q },
        { u"EUC-JP"_q,          u"Japanese"_q },
        { u"ISO-2022-JP"_q,     u"Japanese"_q },
        { u"SHIFT-JIS"_q,       u"Japanese"_q },
        { u"CP949"_q,           u"Korean"_q },
        { u"ISO-2022-KR"_q,     u"Korean"_q },
        { u"CP860"_q,           u"Portuguese"_q },
        { u"CP838"_q,           u"Thai"_q },
        { u"TIS-620"_q,         u"Thai"_q },
        { u"CP857"_q,           u"Turkish"_q },
        { u"ISO-8859-9"_q,      u"Turkish"_q },
        { u"Windows-1254"_q,    u"Turkish"_q },
        { u"Windows-1258"_q,    u"Vietnamese"_q },
        { u"UTF-8"_q,           u"Unicode"_q },
        { u"UTF-16"_q,          u"Unicode"_q },
        { u"UTF-32"_q,          u"Unicode"_q },
        { u"UTF-16BE"_q,        u"Unicode"_q },
        { u"UTF-16LE"_q,        u"Unicode"_q },
        { u"UTF-32BE"_q,        u"Unicode"_q },
        { u"UTF-32LE"_q,        u"Unicode"_q },
        { u"UTF-7"_q,           u"Unicode"_q },
        { u"SCSU"_q,            u"Unicode"_q },
        { u"CESU-8"_q,          u"Unicode"_q },
        { u"BOCU-1"_q,          u"Unicode"_q }
    };
    return all;
}

auto EncodingInfo::categorized() -> QVector<QVector<EncodingInfo>>
{
    QVector<QVector<EncodingInfo>> list;
    QString group, subgroup;
    for (auto &e : EncodingInfo::all()) {
        if (_Change(group, e.group()) | _Change(subgroup, e.subgroup()))
            list.push_back(QVector<EncodingInfo>());
        list.back().push_back(e);
    }
    return list;
}

auto EncodingInfo::codec() const -> QTextCodec*
{
    if (m_name.isEmpty())
        return nullptr;
    return QTextCodec::codecForName(m_name.toLatin1());
}
