#ifndef JRPARSER_HPP
#define JRPARSER_HPP

#include "jrcommon.hpp"

class JrServer;

class JrClient : public QObject {
public:
    JrClient(QIODevice *device, const QString &peer, JrServer *server);
    ~JrClient();
    auto peer() const -> QString;
    auto device() const -> QIODevice*;
    auto server() const -> JrServer*;
    auto parse(const QByteArray &data) -> void;
    virtual auto autoClose() const -> bool { return false; }
    auto reply(const JrResponse &response) -> void;
    auto reply(const QList<JrResponse> &response) -> void;
protected:
    virtual auto beginReply(const QList<JrResponse> &/*responses*/, int /*length*/) -> void { }
    virtual auto endReply() -> void { }
private:
    auto write(const QList<JrResponse> &responses,
               const QJsonDocument &doc) -> void;
    struct Data;
    Data *d;
};

class JrHttp : public JrClient {
public:
    enum Status {
        Ok = 200,
        BadRequest = 400,
        NotFound = 404,
        MethodNotAllowed = 405,
        InternalServerError = 500
    };
    JrHttp(QIODevice *device, const QString &peer, JrServer *server);
    ~JrHttp();
    auto beginReply(const QList<JrResponse> &responses, int length) -> void final;
    auto autoClose() const -> bool final { return true; }
private:
    struct Data;
    Data *d;
};

class JrRaw : public JrClient {
public:
    JrRaw(QIODevice *device, const QString &peer, JrServer *server);
    ~JrRaw();
private:
    auto read() -> void;
    struct Data;
    Data *d;
};

#endif // JRPARSER_HPP
