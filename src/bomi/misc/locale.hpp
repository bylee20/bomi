#ifndef LOCALE_HPP
#define LOCALE_HPP

class Locale
{
public:
    Locale() { }
    Locale(const QString &locale): m_locale(new QLocale(locale)) { }
    Locale(const Locale &rhs);
    Locale(Locale &&rhs);
    ~Locale() { delete m_locale; }
    auto operator = (const Locale &rhs) -> Locale&;
    auto operator = (Locale &&rhs) -> Locale&;
    auto operator == (const Locale &rhs) const -> bool
    {
        if (m_locale && rhs.m_locale)
            return *m_locale == *rhs.m_locale;
        else
            return !m_locale && !rhs.m_locale;
    }
    auto operator != (const Locale &rhs) const -> bool
        { return !operator == (rhs); }
    auto toQt() const -> QLocale { return *m_locale; }
    auto isValid() const -> bool { return m_locale; }
    auto name() const -> QString { return m_locale ? m_locale->name() : QString(); }
    auto nativeName() const -> QString;
    auto isC() const -> bool { return m_locale && m_locale->language() == QLocale::C; }
    auto language() const -> QLocale::Language { return m_locale ? m_locale->language() : QLocale::AnyLanguage; }
    auto toVariant() const -> QVariant { return m_locale ? QVariant::fromValue(*m_locale) : QVariant(); }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto fromVariant(const QVariant &var) -> Locale
        { return var.isValid() ? Locale(var.value<QLocale>()) : Locale(); }
    static auto system() -> Locale { return {QLocale::system()}; }
    static auto c() -> Locale { return {QLocale::c()}; }
    static auto native() -> Locale;
    static auto setNative(const Locale &l) -> void;
    static auto isoToNativeName(const QString &iso) -> QString;

    static auto importIcu() -> void;
private:
    Locale(const QLocale &l): m_locale(new QLocale(l)) { }
    QLocale *m_locale = nullptr;
};

Q_DECLARE_METATYPE(Locale)

auto operator << (QDataStream &out, const ::Locale &l) -> QDataStream&;
auto operator >> (QDataStream &in, ::Locale &l) -> QDataStream&;

#endif // LOCALE_HPP
