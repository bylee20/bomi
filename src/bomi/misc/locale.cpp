#include "locale.hpp"
#include <unicode/locid.h>
#include "misc/log.hpp"

auto operator << (QDataStream &out, const ::Locale &l) -> QDataStream&
{
    out << l.toVariant(); return out;
}

auto operator >> (QDataStream &in, ::Locale &l) -> QDataStream&
{
    QVariant var;
    in >> var;
    l = ::Locale::fromVariant(var);
    return in;
}

DECLARE_LOG_CONTEXT(Locale)

struct Data {
    Data() {
        b2t[u"cze"_q] = u"ces"_q;
        b2t[u"baq"_q] = u"eus"_q;
        b2t[u"fre"_q] = u"fra"_q;
        b2t[u"ger"_q] = u"deu"_q;
        b2t[u"gre"_q] = u"ell"_q;
        b2t[u"arm"_q] = u"hye"_q;
        b2t[u"ice"_q] = u"isl"_q;
        b2t[u"geo"_q] = u"kat"_q;
        b2t[u"mac"_q] = u"mkd"_q;
        b2t[u"mao"_q] = u"mri"_q;
        b2t[u"may"_q] = u"msa"_q;
        b2t[u"bur"_q] = u"mya"_q;
        b2t[u"dut"_q] = u"nld"_q;
        b2t[u"per"_q] = u"fas"_q;
        b2t[u"rum"_q] = u"ron"_q;
        b2t[u"slo"_q] = u"slk"_q;
        b2t[u"alb"_q] = u"sqi"_q;
        b2t[u"tib"_q] = u"bod"_q;
        b2t[u"wel"_q] = u"cym"_q;
        b2t[u"chi"_q] = u"zho"_q;

        // custom code for opensubtitles
        b2t[u"scc"_q] = u"srp"_q;
        b2t[u"pob"_q] = u"por"_q;
        b2t[u"pb"_q]  = u"pt"_q;
    }
    icu::Locale icu;
    ::Locale native = ::Locale::system();
    QHash<QString, QString> b2t;
    QMap<QString, QString> isoName;
};

static auto data() -> Data& { static Data d; return d; }

::Locale::Locale(const Locale &rhs)
{
    if (rhs.m_locale)
        m_locale = new QLocale(*rhs.m_locale);
}

::Locale::Locale(Locale &&rhs)
{
    std::swap(m_locale, rhs.m_locale);
}

auto ::Locale::operator = (const Locale &rhs) -> Locale&
{
    if (this != &rhs) {
        _Delete(m_locale);
        if (rhs.m_locale)
            m_locale = new QLocale(*rhs.m_locale);
    }
    return *this;
}

auto ::Locale::operator = (Locale &&rhs) -> Locale&
{
    if (this != &rhs)
        std::swap(m_locale, rhs.m_locale);
    return *this;
}

auto ::Locale::native() -> Locale
{
    return data().native;
}

auto ::Locale::setNative(const Locale &l) -> void
{
    data().native = l;
    data().icu = icu::Locale::createFromName(l.name().toLatin1().data());
}

auto ::Locale::isoToNativeName(const QString &_iso) -> QString
{
    if (_iso.size() > 3)
        return QString();
    QString iso = _iso.toLower();
    auto it = data().b2t.constFind(iso);
    if (it != data().b2t.cend())
        iso = *it;
    auto &name = data().isoName[iso];
    if (name.isEmpty()) {
        icu::UnicodeString str;
        icu::Locale locale(iso.toLatin1());
        locale.getDisplayLanguage(data().icu, str);
        name.setUtf16((const ushort*)str.getBuffer(), str.length());
        if (iso == name)
            _Error("Cannot find locale for %%", iso);
    }
    return name;
}

auto ::Locale::nativeName() const -> QString
{
    if (!m_locale)
        return QString();
    if (m_locale->language() == QLocale::C)
        return u"C"_q;
    auto l = ::icu::Locale::createFromName(m_locale->name().toLatin1().data());
    icu::UnicodeString str;
    l.getDisplayName(data().icu, str);
    return QString::fromUtf16((const ushort*)str.getBuffer(), str.length());
}

// dummy for pref
auto ::Locale::toJson() const -> QJsonObject
{
    return QJsonObject();
}

auto ::Locale::setFromJson(const QJsonObject &json) -> bool
{
    Q_UNUSED(json);
    return true;
}
