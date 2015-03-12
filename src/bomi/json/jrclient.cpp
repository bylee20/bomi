#include "jrclient.hpp"
#include "jrserver.hpp"
#include "http-parser/http_parser.h"
#include "misc/log.hpp"
#include <QNetworkRequest>

DECLARE_LOG_CONTEXT(JSON-RPC)

struct JrClient::Data {
    QIODevice *device;
    JrServer *server;
    QString peer;
};

JrClient::JrClient(QIODevice *device, const QString &peer, JrServer *server)
    : QObject(server), d(new Data)
{
    d->device = device;
    d->server = server;
    d->peer = peer;
}

JrClient::~JrClient()
{
    delete d;
}

auto JrClient::peer() const -> QString
{
    return d->peer;
}

auto JrClient::reply(const JrResponse &response) -> void
{
    write({ response }, QJsonDocument(response.toJson()));
}

auto JrClient::reply(const QList<JrResponse> &responses) -> void
{
    QJsonArray array;
    for (auto &res : responses)
        array.push_back(res.toJson());
    write(responses, QJsonDocument(array));
}

auto JrClient::write(const QList<JrResponse> &responses,
                     const QJsonDocument &doc) -> void
{
    const auto data = doc.toJson(QJsonDocument::Compact);
    beginReply(responses, data.size() + 1);
    *d->device << data << '\n';
    endReply();
    if (autoClose())
        d->device->close();
}

auto JrClient::parse(const QByteArray &data) -> void
{
    d->server->parse(this, data);
}

auto JrClient::device() const -> QIODevice*
{
    return d->device;
}

auto JrClient::server() const -> JrServer*
{
    return d->server;
}

/******************************************************************************/

using Request = QNetworkRequest;

struct JrHttp::Data {
    JrHttp *p = nullptr;
    http_parser *parser = nullptr;
    http_parser_settings settings;
    Request request;
    QByteArray field, value, body;
    QString url;
    auto fillHeader() -> void
    {
        if (field.isEmpty() || value.isEmpty())
            return;
        request.setRawHeader(field, value);
        field.clear();
        value.clear();
    }
    SIA text(JrHttp::Status status) -> QByteArray
    {
        switch (status) {
        case Ok: return "OK"_b;
        case BadRequest: return "Bad Request"_b;
        case NotFound: return "Not Found"_b;
        case MethodNotAllowed: return "Method Not Allowed"_b;
        case InternalServerError: return "Internal Server Error"_b;
        }
        return QByteArray();
    }
    auto close(JrHttp::Status status) -> void
    {
        writeStatus(status) << "\r\n";
        p->device()->close();
    }
    auto writeStatus(JrHttp::Status status) -> QIODevice&
    {
        return *p->device() << "HTTP/1.1 " << QByteArray::number(status)
                            << " " << text(status) << "\r\n";
    }
};

JrHttp::JrHttp(QIODevice *device, const QString &peer, JrServer *server)
    : JrClient(device, peer, server), d(new Data)
{
    d->p = this;
    d->parser = new http_parser;
    d->parser->data = d;
    http_parser_init(d->parser, HTTP_REQUEST);
    http_parser_settings_init(&d->settings);

#define GET_DATA() static_cast<Data*>(parser->data)
    d->settings.on_message_begin = [] (http_parser *parser) -> int {
        auto d = GET_DATA();
        d->field.clear();
        d->value.clear();
        d->request = Request();
        d->body.clear();
        d->url.clear();
        return 0;
    };
    d->settings.on_url = [] (http_parser *parser, const char *at, size_t len) -> int
        { GET_DATA()->url = QString::fromLatin1(at, len); return 0; };
    d->settings.on_header_field = [] (http_parser *parser, const char *at, size_t len) -> int
        { auto d = GET_DATA(); d->fillHeader(); d->field.append(at, len); return 0; };
    d->settings.on_header_value = [] (http_parser *parser, const char *at, size_t len) -> int
        { GET_DATA()->value.append(at, len); return 0; };
    d->settings.on_headers_complete = [] (http_parser *parser) -> int
    {
        auto d = GET_DATA();
        d->fillHeader();

        switch (parser->method) {
        case HTTP_POST:
            break;
        case HTTP_GET:
            return 0;
        default:
            d->close(MethodNotAllowed);
            return -1;
        }

        const auto type = d->request.header(Request::ContentTypeHeader).toByteArray();
        const auto len = d->request.header(Request::ContentLengthHeader).toInt();
        const auto accept = d->request.rawHeader("Accept");

        static const QList<QByteArray> types= {
            "application/json-rpc",
            "application/json",
            "application/jsonrequest"
        };
        if (len <= 0 || !types.contains(type) || !types.contains(accept)) {
            d->close(BadRequest);
            _Error("Bad Request: content-type: %%, content-length: %%, accept: %%", type, len, accept);
            return -1;
        }
        return 0;
    };
    d->settings.on_body = [] (http_parser *parser, const char *at, size_t len) -> int
        { GET_DATA()->body.append(at, len); return 0; };
    d->settings.on_message_complete = [] (http_parser *parser) -> int {
        auto d = GET_DATA();
        if (parser->method == HTTP_GET) {
            QRegEx rx(uR"((\?|&)([^=]+)=([^&]+))"_q);
            int pos = 0;
            d->body.clear();
            d->body += "{\"jsonrpc\":\"2.0\"";
            while (pos < d->url.size()) {
                const auto m = rx.match(d->url, pos);
                if (!m.hasMatch())
                    break;
                if (d->body.at(d->body.size() - 1) != ',')
                    d->body += ',';
                const auto key = m.capturedRef(2).toLatin1();
                const auto value = m.capturedRef(3).toLatin1();
                if (key == "jsonrpc"_b)
                    d->body.replace("{\"jsonrpc\":\"2.0\"", "{\"jsonrpc\":\"" + value + "\"");
                else {
                    d->body += '"' + key + "\":";
                    if (key == "params"_b) {
                        const auto base64 = QByteArray::fromPercentEncoding(value);
                        d->body += QByteArray::fromBase64(base64);
                    } else
                        d->body += '"' + value + '"';
                }
                pos = m.capturedEnd();
                if (pos < 0)
                    break;
            }
            d->body += '}';
        }
        d->p->parse(d->body);
        return 0;
    };
#undef GET_DATA
    connect(device, &QIODevice::readyRead, this, [=] () {
        const auto data = this->device()->readAll();
        http_parser_execute(d->parser, &d->settings, data.data(), data.size());
    });
}

JrHttp::~JrHttp()
{
    delete d->parser;
    delete d;
}

auto JrHttp::beginReply(const QList<JrResponse> &responses, int length) -> void
{
    Status status = Ok;
    for (auto &res : responses) {
        switch (res.error.code) {
        case JrError::NoError:
            break;
        case JrError::InvalidRequest:
            status = BadRequest;
            break;
        case JrError::MethodNotFound:
            status = NotFound;
            break;
        default:
            status = InternalServerError;
            break;
        }
        if (status != Ok)
            break;
    }
    d->writeStatus(status) << "Content-Type: application/json-rpc\r\n"
                           << "Content-Length: " << length << "\r\n\r\n";
}

/******************************************************************************/

struct JrRaw::Data {
    QByteArray data;
    char bracket_l = 0, bracket_r = 0;
    int last = 0, open = 0, begin = -1;
    bool inString = false;
    auto extract() -> QByteArray
    {
        const char* at = data.constData() + last;
        const char* end = at + data.size();
        if (begin < 0) {
            Q_ASSERT(!open);
            Q_ASSERT(!bracket_l);
            Q_ASSERT(!bracket_r);
            Q_ASSERT(!inString);
            while (at < end) {
                if (*at == '{') {
                    bracket_l = '{';
                    bracket_r = '}';
                    break;
                }
                if(*at == '[') {
                    bracket_l = '[';
                    bracket_r = ']';
                    break;
                }
                ++at;
            }
            if (at >= end) {
                data.clear();
                last = 0;
                return QByteArray();
            }
            Q_ASSERT(bracket_l && bracket_r);
            open = 1;
            begin = at - data.constData();
            ++at;
        }

        while (open > 0 && at < end) {
            if (*at == '\\')
                ++at;
            else if (*at == '"')
                inString = !inString;
            else if (!inString) {
                if (*at == bracket_l)
                    ++open;
                else if (*at == bracket_r)
                    --open;
            }
            ++at;
        }

        const auto pos = at - data.constData();
        if (!open) { // found
            Q_ASSERT(!inString);
            Q_ASSERT(begin >= 0);
            auto ret = data.mid(begin, pos - begin);
            data = data.mid(pos);
            last = bracket_l = bracket_r = 0;
            begin = -1;
            return ret;
        }
        last = pos;
        return QByteArray();
    }
};

JrRaw::JrRaw(QIODevice *device, const QString &peer, JrServer *server)
    : JrClient(device, peer, server), d(new Data)
{
    connect(device, &QIODevice::readyRead, this, &JrRaw::read);
}

JrRaw::~JrRaw()
{
    delete d;
}

auto JrRaw::read() -> void
{
    Q_ASSERT(device());
    d->data.append(device()->readAll());
    while (!d->data.isEmpty()) {
        auto data = d->extract();
        if (data.isEmpty())
            return; // fetch more
        parse(data);
    }
}
