#ifndef LOCALCONNECTION_HPP
#define LOCALCONNECTION_HPP

class LocalConnection : public QObject {
    Q_OBJECT
public:
    LocalConnection(const QString &id, QObject *parent = 0);
    ~LocalConnection();
    auto runServer() -> bool;
    auto sendMessage(const QByteArray &message, int timeout) -> bool;
signals:
    void messageReceived(const QByteArray &message);
private:
    struct Data;
    Data *d;
};

#endif // LOCALCONNECTION_HPP
