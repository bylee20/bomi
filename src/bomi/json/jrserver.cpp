#include "jrserver.hpp"
#include "jrclient.hpp"
#include "jriface.hpp"
#include "misc/log.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QSslSocket>
#include <QLocalSocket>
#include <QLocalServer>

DECLARE_LOG_CONTEXT(JSON-RPC)

using ServerError = QAbstractSocket::SocketError;

class JrTransport {
public:
    JrTransport(JrServer *server): m_server(server) { }
    virtual ~JrTransport() { }
    auto addClient(QIODevice *device, const QString &peer) -> bool
        { return m_server->addClient(device, peer); }
    auto removeClient(QIODevice *device) -> void
        { m_server->removeClient(device); }
    auto server() const -> JrServer* { return m_server; }
    auto error(ServerError error, const QString &str) -> void
        { m_server->sendError(error, str); }
    virtual auto listen(const QString &address, int port) -> bool = 0;
    virtual auto serverName() const -> QString = 0;
private:
    JrServer *m_server;
};

struct JrTcp : public QTcpServer, public JrTransport {
    JrTcp(JrServer *server): JrTransport(server) { }
    auto incomingConnection(qintptr sd) -> void final
    {
        QTcpSocket *s = nullptr;
        switch (server()->connection()) {
        case JrConnection::Tcp:
            s = new QTcpSocket(this);
            break;
//        case JrConnection::Ssl:
//            s = new QSslSocket(this);
//            break;
        default:
            return;
        }
        if (!s->setSocketDescriptor(sd)) {
            _Error("Cannot set TCP socket descriptor.");
            s->deleteLater();
            return;
        }
        if (!addClient(s, s->peerAddress().toString() % ':'_q % _N(s->peerPort()))) {
            s->deleteLater();
            return;
        }
        connect(SIGNAL_VT(s, error, ServerError), this,
                [=] (ServerError e) { error(e, errorString()); });
        connect(s, &QTcpSocket::disconnected, this,
                [=] () { removeClient(s); s->deleteLater(); });
    }
    auto listen(const QString &address, int port) -> bool final
    {
        if (!address.compare("localhost"_a, Qt::CaseInsensitive))
            return QTcpServer::listen(QHostAddress::LocalHost, port);
        else
            return QTcpServer::listen(QHostAddress(address), port);
    }
    auto serverName() const -> QString final
        { return serverAddress().toString() % ':'_q % _N(serverPort()); }
};

struct JrLocal : public QLocalServer, public JrTransport {
    JrLocal(JrServer *server): JrTransport(server) { }
    auto incomingConnection(quintptr sd) -> void final
    {
        auto s = new QLocalSocket(this);
        if (!s->setSocketDescriptor(sd)) {
            _Error("Cannot set local socket descriptor.");
            s->deleteLater();
            return;
        }
        if (!addClient(s, QString::number(s->socketDescriptor()))) {
            s->deleteLater();
            return;
        }
        connect(s, &QLocalSocket::disconnected, this,
                [=] () { removeClient(s); s->deleteLater(); });
    }
    auto listen(const QString &address, int /*port*/) -> bool final
    {
        if (QLocalServer::listen(address))
            return true;
        error(serverError(), errorString());
        return false;
    }
    auto serverName() const -> QString final { return fullServerName(); }
};

/******************************************************************************/

struct JrServer::Data {
    JrConnection connection = JrConnection::Tcp;
    JrProtocol protocol = JrProtocol::Http;
    JrTransport *transport = nullptr;
    JrIface *iface = nullptr;
    ServerError error = QAbstractSocket::UnknownSocketError;
    QMap<QIODevice*, JrClient*> clients;
    Error handleError;
    QString errorString = u"No Error"_q;
};

JrServer::JrServer(JrConnection connection, JrProtocol protocol, QObject *parent)
    : QObject(parent), d(new Data)
{
    d->connection = connection;
    d->protocol = protocol;
    switch (d->connection) {
    case JrConnection::Tcp:
//    case JrConnection::Ssl:
        d->transport = new JrTcp(this);
        break;
    case JrConnection::Local:
        d->transport = new JrLocal(this);
        break;
    }
}

JrServer::~JrServer()
{
    _Info("Closing server.");
    setInterface(nullptr);
    auto devices = d->clients.keys();
    for (auto dev : devices) {
        dev->close();
        removeClient(dev);
    }
    delete d->transport;
    delete d;
}

auto JrServer::connection() const -> JrConnection
{
    return d->connection;
}

auto JrServer::protocol() const -> JrProtocol
{
    return d->protocol;
}

auto JrServer::listen(const QString &address, int port) -> bool
{
    d->error = QAbstractSocket::UnknownSocketError;
    d->errorString = u"No Error"_q;
    if (d->transport && d->transport->listen(address, port)) {
        _Info("Listening %%.", d->transport->serverName());
        return true;
    }
    _Error("Failed to listen '%%:%%': %%", address, port, errorString());
    return false;
}

auto JrServer::parse(JrClient *client, const QByteArray &data) -> void
{
    QJsonParseError error = { 0, QJsonParseError::NoError };
    auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        _Error("Cannot parse JSON: %%", error.errorString());
        client->reply(_JrErrorResponse(QJsonValue::Null, JrError::ParseError,
                                       error.errorString()));
        return;
    }

    QJsonArray array;
    if (doc.isObject())
        array.push_back(doc.object());
    else if (doc.isArray())
        array = doc.array();

    QList<JrResponse> replies;
    replies.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        const auto request = JrRequest::fromJson(array.at(i).toObject());
        if (!request.isValid()) {
            _Error("Invalid request object exits.");
            replies.push_back(_JrErrorResponse(QJsonValue::Null, JrError::InvalidRequest));
        } else {
            JrResponse res;
            if (d->iface)
                res = d->iface->request(request);
            else
                res = _JrErrorResponse(request.id(), JrError::MethodNotFound);
            if (!request.isNotification())
                replies.push_back(res);
        }
    }
    if (replies.size() == 1)
        client->reply(replies.front());
    else
        client->reply(replies);
}

auto JrServer::addClient(QIODevice *dev, const QString &peer) -> bool
{
    JrClient *client = nullptr;
    Q_ASSERT(!client);
    switch (d->protocol) {
    case JrProtocol::Http:
        client = new JrHttp(dev, peer, this);
        break;
    case JrProtocol::Raw:
        client = new JrRaw(dev, peer, this);
        break;
    default:
        return false;
    }
    _Info("Client connected: %%", peer);
    d->clients[dev] = client;
    return true;
}

auto JrServer::removeClient(QIODevice *dev) -> void
{
    auto client = d->clients.take(dev);
    if (client) {
        _Info("Client disconnected: %%", client->peer());
        delete client;
    }
}

auto JrServer::setInterface(JrIface *iface) -> void
{
    if (d->iface)
        disconnect(d->iface, nullptr, this, nullptr);
    d->iface = iface;
    if (d->iface)
        connect(d->iface, &JrIface::destroyed, this,
                [=] () { if (d->iface == iface) d->iface = nullptr; });
}

auto JrServer::setErrorHandler(Error &&func) -> void
{
    d->handleError = std::move(func);
}

auto JrServer::lastError() const -> ServerError
{
    return d->error;
}

auto JrServer::errorString() const -> QString
{
    return d->errorString;
}

auto JrServer::sendError(ServerError error, const QString &errorString) -> void
{
    if (error != QAbstractSocket::RemoteHostClosedError) {
        d->error = error;
        d->errorString = errorString;
        if (d->handleError)
            d->handleError(error);
    }
}

auto JrServer::serverName() const -> QString
{
    return d->transport ? d->transport->serverName() : QString();
}
