#ifndef JSONSTORAGE_HPP
#define JSONSTORAGE_HPP

#include <QJsonParseError>

class JsonStorage {
public:
    enum Error { NoError, NoFile, OpenError, ParseError };
    JsonStorage() noexcept { m_error = NoFile; }
    JsonStorage(const QString &fileName) noexcept;
    auto setFileName(const QString &fileName) noexcept -> void
    { m_fileName = fileName; m_error = NoError; }
    auto fileName() const noexcept -> QString { return m_fileName; }
    auto printError() const -> bool;
    auto write(const QJsonObject &json) noexcept -> bool;
    auto read() noexcept -> QJsonObject;
    auto hasError() const noexcept -> bool { return m_error != NoError; }
    auto error() const noexcept -> Error  { return m_error; }
private:
    auto setError(Error error) noexcept -> void;
    QString m_fileName;
    Error m_error = NoError;
    QJsonParseError m_parseError;
};

auto _JsonToQObject(const QJsonObject &json, QObject *obj) -> bool;
auto _JsonFromQObject(const QObject *obj) -> QJsonObject;

auto _JsonFromQVariant(const QVariant &var) -> QJsonValue;
auto _JsonToQVariant(const QJsonValue &json, const QVariant &def) -> QVariant;
auto _JsonToQVariant(const QJsonValue &json, int metaType) -> QVariant;
auto _JsonToQVariant(const QJsonValue &json, int metaType, const QVariant &def) -> QVariant;

auto _JsonType(int metaType) -> QJsonValue::Type;
auto _QVariantFromType(int metaType) -> QVariant;

#endif // JSONSTORAGE_HPP
