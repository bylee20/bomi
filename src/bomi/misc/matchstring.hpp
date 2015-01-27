#ifndef MATCHSTRING_HPP
#define MATCHSTRING_HPP

template<class T> struct JsonIO;

class MatchString {
public:
    MatchString() { }
    MatchString(const QString &text, bool regex = false)
        : m_text(text), m_regex(regex) { }
    auto caseSensitivity() const
        { return m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive; }
    auto isCaseInsensitive() const { return !m_caseSensitive; }
    auto isCaseSensitive() const { return m_caseSensitive; }
    auto isRegEx() const { return m_regex; }
    auto setCaseInsensitive(bool cis) { m_caseSensitive = !cis; }
    auto setCaseSensitive(bool cs) { m_caseSensitive = cs; }
    auto setRegEx(bool rx) { m_regex = rx; }
    auto setString(const QString &text) { m_text = text; }
    auto string() const { return m_text; }
    auto match(const QString &path) const -> bool;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
private:
    friend struct JsonIO<MatchString>;
    QString m_text;
    bool m_regex = false;
    bool m_caseSensitive = false;
};

#endif // MATCHSTRING_HPP
