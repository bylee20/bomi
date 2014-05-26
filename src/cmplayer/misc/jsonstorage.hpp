#ifndef JSONSTORAGE_HPP
#define JSONSTORAGE_HPP

#include "stdafx.hpp"

class JsonStorage {
public:
    enum Error { NoError, NoFile, OpenError, ParseError };
    JsonStorage() noexcept { m_error = NoFile; }
    JsonStorage(const QString &fileName) noexcept;
    auto setFileName(const QString &fileName) noexcept -> void
    { m_fileName = fileName; m_error = NoError; }
    auto fileName() const noexcept -> QString { return m_fileName; }
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

#endif // JSONSTORAGE_HPP
