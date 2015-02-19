#include "matchstring.hpp"
#include "misc/json.hpp"

auto MatchString::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert(u"text"_q, m_text);
    json.insert(u"regex"_q, m_regex);
    json.insert(u"case-sensitive"_q, m_caseSensitive);
    return json;
}

auto MatchString::setFromJson(const QJsonObject &json) -> bool
{
    m_text = json[u"text"_q].toString();
    m_regex = json[u"regex"_q].toBool();
    m_caseSensitive = json[u"case-sensitive"_q].toBool();
    return true;
}

auto MatchString::isValid() const -> bool
{
    return !m_regex || checkRegEx();
}

auto MatchString::checkRegEx() const -> bool
{
    if (m_cache.pattern().isEmpty()) {
        m_cache.setPattern(m_text);
        m_cache.setPatternOptions(m_caseSensitive ? QRegEx::NoPatternOption
                                                  : QRegEx::CaseInsensitiveOption);
    }
    return m_cache.isValid();
}

auto MatchString::match(const QString &path) const -> bool
{
    if (m_regex) {
        checkRegEx();
        return m_cache.match(path).hasMatch();
    } else
        return !m_text.compare(path, caseSensitivity());
}

auto MatchString::contains(const QString &text) const -> bool
{
    if (m_regex) {
        checkRegEx();
        return text.contains(m_cache);
    } else
        return text.contains(m_text, caseSensitivity());
}
