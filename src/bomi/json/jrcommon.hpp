#ifndef JRCOMMON_HPP
#define JRCOMMON_HPP

class JrRequest {
public:
    auto id() const -> QJsonValue { return m_id; }
    auto params() const -> QJsonValue { return m_params; }
    auto version() const -> QString { return m_version; }
    auto method() const -> QString { return m_method; }
    auto isValid() const -> bool;
    auto isNotification() const -> bool { return m_id.isUndefined(); }
    auto hasParams() const -> bool { return !m_params.isUndefined(); }
    static auto fromJson(const QJsonObject &json) -> JrRequest;
private:
    QString m_version, m_method;
    QJsonValue m_params{QJsonValue::Undefined}, m_id{QJsonValue::Undefined};
};

enum class JrError {
    NoError = 0,
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ServerErrorBegin = -32099,
    ServerErrorEnd = -32000
};

class JrResponse {
public:
    JrResponse() { }
    JrResponse(const JrRequest &req, const QJsonValue &result = QJsonValue::Undefined)
        : id(req.id()), result(result) { }
    QJsonValue id{QJsonValue::Undefined};
    QJsonValue result{QJsonValue::Undefined};
    struct {
        JrError code = JrError::NoError;
        QString message;
        QJsonValue data{QJsonValue::Undefined};
    } error;
    auto isError() const -> bool { return error.code != JrError::NoError; }
    auto toJson() const -> QJsonObject;
};

auto _JrErrorResponse(const QJsonValue &id, JrError e,
                      const QString &msg = QString(),
                      const QJsonValue &data = QJsonValue::Undefined) -> JrResponse;

#endif // JRCOMMON_HPP
