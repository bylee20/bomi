#include "encodinginfo.hpp"
#include <QTextCodec>
#include "misc/log.hpp"
#include "misc/charsetdetector.hpp"

auto operator << (QDataStream &out, const EncodingInfo &e) -> QDataStream&
{
    out << e.mib(); return out;
}

auto operator >> (QDataStream &in, EncodingInfo &e) -> QDataStream&
{
    int mib = 0;
    in >> mib;
    e = EncodingInfo::fromMib(mib);
    return in;
}

DECLARE_LOG_CONTEXT(Encoding)

EncodingInfo::EncodingInfo()
{

}

EncodingInfo::EncodingInfo(int mib, const QString &name,
                           const QString &group, const QString &sub)
    : m_mib(mib), m_name(name), m_group(group), m_subgroup(sub)
{

}

EncodingInfo::~EncodingInfo()
{

}

auto EncodingInfo::detect(Category c, const QByteArray &data) -> EncodingInfo
{
    return detect(c, default_(c), data);
}

auto EncodingInfo::detect(Category c, const QString &fileName, int length) -> EncodingInfo
{
    return detect(c, default_(c), fileName, length);
}

auto EncodingInfo::detect(Category c, const EncodingInfo &fb, const QByteArray &data) -> EncodingInfo
{
    const auto conf = _confidence(c);
    if (conf < 0 && fb.isValid())
        return fb;
    const auto ret = CharsetDetector::detect(data, conf);
    return (!fb.isValid() || ret.isValid()) ? ret : fb;
}

auto EncodingInfo::detect(Category c, const EncodingInfo &fb, const QString &fileName, int length) -> EncodingInfo
{
    const auto conf = _confidence(c);
    if (conf < 0 && fb.isValid())
        return fb;
    const auto ret = CharsetDetector::detect(fileName, conf, length);
    return (!fb.isValid() || ret.isValid()) ? ret : fb;
}

auto EncodingInfo::_confidence(Category c) -> double&
{
    static QVector<double> confs;
    if (confs.empty()) {
        confs.resize(CategoryMax);
        confs.fill(-1);
    }
    return confs[c];
}

auto EncodingInfo::setDefault(Category c, const EncodingInfo &def, double autodetect) -> void
{
    _default(c) = def;
    _confidence(c) = autodetect;
}

auto EncodingInfo::_default(Category c) -> EncodingInfo&
{
    static QVector<EncodingInfo> defs;
    if (defs.empty()) {
        defs.resize(CategoryMax);
        defs.fill(utf8());
    }
    return defs[c];
}

auto EncodingInfo::fromMib(int mib) -> EncodingInfo
{
    if (mib <= 0)
        return EncodingInfo();
    for (auto &e : all()) {
        if (e.mib() == mib)
            return e;
    }
    _Warn("'%%' is not registered mib number.", mib);
    return fromCodec(QTextCodec::codecForMib(mib));
}

auto EncodingInfo::fromName(const QString &name) -> EncodingInfo
{
    if (name.isEmpty())
        return EncodingInfo();
    for (auto &e : all()) {
        if (!e.name().compare(name, Qt::CaseInsensitive))
            return e;
    }
    _Warn("'%%' is not registered name.", name);
    return fromCodec(QTextCodec::codecForName(name.toLatin1()));
}

auto EncodingInfo::fromCodec(const QTextCodec *codec) -> EncodingInfo
{
    if (codec) {
        for (auto &e : all()) {
            if (e.codec() == codec)
                return e;
        }
    }
    return EncodingInfo();
}

auto EncodingInfo::codec() const -> QTextCodec*
{
    if (m_mib < 0)
        return nullptr;
    return QTextCodec::codecForMib(m_mib);
}

auto EncodingInfo::toJson() const -> QJsonValue
{
    return m_mib;
}

auto EncodingInfo::setFromJson(const QJsonValue &json) -> bool
{
    EncodingInfo e;
    if (json.type() == QJsonValue::String)
        e = fromName(json.toString());
    else
        e = fromMib(qRound(json.toDouble()));
    if (!e.isValid())
        return false;
    *this = e;
    return true;
}

auto EncodingInfo::all() -> const QVector<EncodingInfo>&
{
    static const QVector<EncodingInfo> all = {
        { 2051, u"CP864"_q,           u"Arabic"_q },
        { 9,    u"ISO-8859-6"_q,      u"Arabic"_q },
        { 2256, u"Windows-1256"_q,    u"Arabic"_q },
        { 2087, u"CP775"_q,           u"Baltic"_q },
        { 7,    u"ISO-8859-4"_q,      u"Baltic"_q },
        { 109,  u"ISO-8859-13"_q,     u"Baltic"_q },
        { 2257, u"Windows-1257"_q,    u"Baltic"_q },
        { 110,  u"ISO-8859-14"_q,     u"Celtic"_q },
        { 113,  u"GBK"_q,             u"Chinese"_q, u"Simplified"_q },
        { 2085, u"HZ-GB-2312"_q,      u"Chinese"_q, u"Simplified"_q },
        { 114,  u"GB18030"_q,         u"Chinese"_q, u"Simplified"_q },
        { 2026, u"Big5"_q,            u"Chinese"_q, u"Traditional"_q },
        { 2101, u"Big5-HKSCS"_q,      u"Chinese"_q, u"Traditional"_q },
        { 104,  u"ISO-2022-CN"_q,     u"Chinese"_q, u"Traditional"_q },
        { 105,  u"ISO-2022-CN-EXT"_q, u"Chinese"_q, u"Traditional"_q },
        { 2046, u"CP855"_q,           u"Cyrillic"_q },
        { 2086, u"CP866"_q,           u"Cyrillic"_q },
        { 8,    u"ISO-8859-5"_q,      u"Cyrillic"_q },
        { 2084, u"KOI8-R"_q,          u"Cyrillic"_q },
        { 2088, u"KOI8-U"_q,          u"Cyrillic"_q },
        { 2251, u"Windows-1251"_q,    u"Cyrillic"_q },
        { 2010, u"CP852"_q,           u"European"_q, u"Central"_q },
        { 5,    u"ISO-8859-2"_q,      u"European"_q, u"Central"_q },
        { 2250, u"Windows-1250"_q,    u"European"_q, u"Central"_q },
        { 2049, u"CP861"_q,           u"European"_q, u"Icelandic"_q },
        { 2052, u"CP865"_q,           u"European"_q, u"Nordic"_q },
        { 13,   u"ISO-8859-10"_q,     u"European"_q, u"Nordic"_q },
        { 6,    u"ISO-8859-3"_q,      u"European"_q, u"South"_q },
        { 2089, u"CP858"_q,           u"European"_q, u"Western"_q },
        { 2009, u"CP850"_q,           u"European"_q, u"Western"_q },
        { 4,    u"ISO-8859-1"_q,      u"European"_q, u"Western"_q },
        { 111,  u"ISO-8859-15"_q,     u"European"_q, u"Western"_q },
        { 2027, u"Macintosh"_q,       u"European"_q, u"Western"_q },
        { 2252, u"Windows-1252"_q,    u"European"_q, u"Western"_q },
        { 2050, u"CP863"_q,           u"French"_q, u"Quebec"_q },
        { 2054, u"CP869"_q,           u"Greek"_q },
        { 10,   u"ISO-8859-7"_q,      u"Greek"_q },
        { 2253, u"Windows-1253"_q,    u"Greek"_q },
        { 2013, u"DOS-862"_q,         u"Hebrew"_q },
        { 11,   u"ISO-8859-8"_q,      u"Hebrew"_q },
        { 2255, u"Windows-1255"_q,    u"Hebrew"_q },
        { 16,   u"ISO-2022-JP-1"_q,   u"Japanese"_q },
        { 40,   u"ISO-2022-JP-2"_q,   u"Japanese"_q },
        { 18,   u"EUC-JP"_q,          u"Japanese"_q },
        { 39,   u"ISO-2022-JP"_q,     u"Japanese"_q },
        { 17,   u"SHIFT-JIS"_q,       u"Japanese"_q },
        { 36,   u"Windows-949"_q,     u"Korean"_q },
        { 37,   u"ISO-2022-KR"_q,     u"Korean"_q },
        { 2048, u"CP860"_q,           u"Portuguese"_q },
        { 2016, u"CP838"_q,           u"Thai"_q },
        { 2259, u"TIS-620"_q,         u"Thai"_q },
        { 2047, u"CP857"_q,           u"Turkish"_q },
        { 12,   u"ISO-8859-9"_q,      u"Turkish"_q },
        { 2254, u"Windows-1254"_q,    u"Turkish"_q },
        { 2258, u"Windows-1258"_q,    u"Vietnamese"_q },
        { 106,  u"UTF-8"_q,           u"Unicode"_q },
        { 1015, u"UTF-16"_q,          u"Unicode"_q },
        { 1017, u"UTF-32"_q,          u"Unicode"_q },
        { 1013, u"UTF-16BE"_q,        u"Unicode"_q },
        { 1014, u"UTF-16LE"_q,        u"Unicode"_q },
        { 1018, u"UTF-32BE"_q,        u"Unicode"_q },
        { 1019, u"UTF-32LE"_q,        u"Unicode"_q },
        { 1012, u"UTF-7"_q,           u"Unicode"_q },
        { 1011, u"SCSU"_q,            u"Unicode"_q },
        { 1016, u"CESU-8"_q,          u"Unicode"_q },
        { 1020, u"BOCU-1"_q,          u"Unicode"_q }
    };
    return all;
}

auto EncodingInfo::description() const -> QString
{
   if (m_subgroup.isEmpty())
       return m_group % ": "_a % m_name;
   return m_group % " ("_a % m_subgroup % "): "_a % m_name;
}

auto EncodingInfo::grouped() -> QVector<QVector<EncodingInfo>>
{
    QVector<QVector<EncodingInfo>> list;
    QString group, subgroup;
    for (auto e : EncodingInfo::all()) {
        if (_Change(group, e.group()) | _Change(subgroup, e.subgroup()))
            list.push_back(QVector<EncodingInfo>());
        list.back().push_back(e);
    }
    return list;
}
