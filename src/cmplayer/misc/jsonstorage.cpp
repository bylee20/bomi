#include "jsonstorage.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(JSON)

JsonStorage::JsonStorage(const QString &fileName) noexcept
    : m_fileName(fileName)
{

}

auto JsonStorage::write(const QJsonObject &json) noexcept -> bool
{
    setError(NoError);
    QFile file(m_fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        setError(OpenError);
        return false;
    }
    QJsonDocument doc(json);
    file.write(doc.toJson());
    return true;
}

auto JsonStorage::setError(Error error) noexcept -> void
{
    m_error = error;
    switch (m_error) {
    case NoFile:
        _Error("Error: '%%' file doesn't exist", m_fileName);
        break;
    case OpenError:
        _Error("Error: Cannot open '%%' file", m_fileName);
        break;
    case ParseError:
        _Error("Error: Cannot parse '%%' file at %%(%%)",
               m_fileName, m_parseError.offset, m_parseError.errorString());
        break;
    default:
        break;
    }
}

auto JsonStorage::read() noexcept -> QJsonObject
{
    setError(NoError);
    QFile file(m_fileName);
    if (!file.exists()) {
        setError(NoFile);
        return QJsonObject();
    }
    if (!file.open(QFile::ReadOnly)) {
        setError(OpenError);
        return QJsonObject();
    }
    const auto json = QJsonDocument::fromJson(file.readAll(), &m_parseError);
    if (m_parseError.error)
        setError(ParseError);
    return json.object();
}
