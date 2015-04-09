#ifndef JRSERVER_HPP
#define JRSERVER_HPP

#include "enum/jrconnection.hpp"
#include "enum/jrprotocol.hpp"
#include <QAbstractSocket>

class JrIface;                          class JrClient;
class JrRequest;                        class JrResponse;

class JrServer : public QObject {
    Q_OBJECT
    using Error = std::function<void(QAbstractSocket::SocketError)>;
public:
    JrServer(JrConnection connection, JrProtocol protocol, QObject *parent = nullptr);
    ~JrServer();
    auto connection() const -> JrConnection;
    auto protocol() const -> JrProtocol;
    auto listen(const QString &address, int port = 2020) -> bool;
    auto setInterface(JrIface *iface) -> void;
    auto serverName() const -> QString;
    auto lastError() const -> QAbstractSocket::SocketError;
    auto errorString() const -> QString;
    auto setErrorHandler(Error &&func) -> void;
private:
    auto sendError(QAbstractSocket::SocketError error,
                   const QString &errorString) -> void;
    auto parse(JrClient *client, const QByteArray &data) -> void;
    auto addClient(QIODevice *dev, const QString &peer = QString()) -> bool;
    auto removeClient(QIODevice *dev) -> void;
    friend class JrTransport;
    friend class JrClient;
    struct Data;
    Data *d;
};

#endif // JRSERVER_HPP
