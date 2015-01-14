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

auto MatchString::match(const QString &path) const -> bool
{
    if (m_regex) {
        QRegEx rx(m_text, m_caseSensitive ? QRegEx::NoPatternOption
                                          : QRegEx::CaseInsensitiveOption);
        return rx.match(path).hasMatch();
    } else
        return !m_text.compare(path, caseSensitivity());
}
