#ifndef MATCHSTRING_HPP
#define MATCHSTRING_HPP

template<class T> struct JsonIO;

class MatchString {
public:
    MatchString() { }
    MatchString(const QString &text, bool regex = false)
        : m_text(text), m_regex(regex) { }
    DECL_EQ(MatchString, &T::m_text, &T::m_regex, &T::m_caseSensitive)
    auto caseSensitivity() const
        { return m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive; }
    auto isCaseInsensitive() const { return !m_caseSensitive; }
    auto isCaseSensitive() const { return m_caseSensitive; }
    auto isRegEx() const { return m_regex; }
    auto setCaseInsensitive(bool cis) -> void { setCaseSensitive(!cis); }
    auto setCaseSensitive(bool cs) -> void { m_caseSensitive = cs; clearRegEx(); }
    auto setRegEx(bool rx) { m_regex = rx; clearRegEx(); }
    auto setString(const QString &text) { m_text = text; clearRegEx(); }
    auto string() const { return m_text; }
    auto match(const QString &path) const -> bool;
    auto contains(const QString &text) const -> bool;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto isValid() const -> bool;
private:
    auto clearRegEx() const -> void { m_cache = QRegEx(); }
    auto checkRegEx() const -> bool;
    friend struct JsonIO<MatchString>;
    QString m_text;
    bool m_regex = false;
    bool m_caseSensitive = false;
    mutable QRegEx m_cache;
};

#endif // MATCHSTRING_HPP
