#include "jrcommon.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(JSON-RPC)

auto JrRequest::fromJson(const QJsonObject &json) -> JrRequest
{
    JrRequest jr;
    jr.m_version = json[u"jsonrpc"_q].toString();
    jr.m_method = json[u"method"_q].toString();
    jr.m_params = json[u"params"_q];
    jr.m_id = json[u"id"_q];
    return jr;
}

auto JrRequest::isValid() const -> bool
{
    return m_version == "2.0"_a && !m_method.isEmpty()
        && _IsOneOf(m_params.type(), QJsonValue::Undefined, QJsonValue::Object, QJsonValue::Array);
}

/******************************************************************************/

auto JrResponse::toJson() const -> QJsonObject
{
    QJsonObject json;
    json[u"jsonrpc"_q] = u"2.0"_q;
    if (isError()) {
        QJsonObject je;
        je[u"code"_q] = (int)error.code;
        je[u"message"_q] = error.message;
        if (!error.data.isUndefined())
            je[u"data"_q] = error.data;
        json[u"error"_q] = je;
    } else {
        if (result.isUndefined()) {
            _Warn("'result' should be set for non-error response.");
            json[u"result"_q] = QJsonValue(QJsonValue::Null);
        } else
            json[u"result"_q] = result;
    }
    if (id.isUndefined()) {
        _Warn("'id' should be set for response.");
        json[u"id"_q] = QJsonValue(QJsonValue::Null);
    } else
        json[u"id"_q] = id;
    return json;
}

/******************************************************************************/

auto _JrErrorResponse(const QJsonValue &id, JrError e, const QString &msg,
                     const QJsonValue &data) -> JrResponse
{
    JrResponse res;
    res.id = id;
    res.error.code = e;
    if (msg.isEmpty()) {
        res.error.message = [&] () {
            switch (e) {
            case JrError::ParseError:
                return u"Parse error"_q;
            case JrError::InvalidParams:
                return u"Invalid parameter(s)"_q;
            case JrError::InvalidRequest:
                return u"Invalid request"_q;
            case JrError::MethodNotFound:
                return u"Method not found"_q;
            case JrError::NoError:
                return QString();
            case JrError::InternalError:
            default:
                return u"Internal error"_q;
            }
        }();
    } else
        res.error.message = msg;
    res.error.data = data;
    return res;
}
