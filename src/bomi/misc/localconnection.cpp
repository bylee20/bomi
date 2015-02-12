#include "localconnection.hpp"
#include <QLocalServer>
#include <QLockFile>
#include <QLocalSocket>

#if defined(Q_OS_WIN)
#include <QtCore/QLibrary>
#include <QtCore/qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif
#if defined(Q_OS_UNIX)
#include <unistd.h>
#endif

static int getUid() {
#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId) {
        QLibrary lib(u"kernel32"_q);
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId) {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        return sessionId;
    }
    return 0;
#else
    return ::getuid();
#endif
}

struct LocalConnection::Data {
    QString id, socket;
    QLocalServer server;
    QLockFile *lock = nullptr;
};

constexpr static const char* ack = "ack";

LocalConnection::LocalConnection(const QString &id, QObject* parent)
: QObject(parent), d(new Data) {
    d->id = id;
    d->socket = id % '-'_q % QString::number(getUid(), 16);
    d->lock = new QLockFile(QDir::temp().path() % '/'_q % d->socket % u"-lock"_q);
    d->lock->setStaleLockTime(0);
}

LocalConnection::~LocalConnection() {
    delete d->lock;
    delete d;
}

auto LocalConnection::runServer() -> bool
{
    if (d->lock->isLocked())
        return true;
    if (!d->lock->tryLock() && !(d->lock->removeStaleLockFile() && d->lock->tryLock()))
        return false;
    if (!d->server.listen(d->socket)) {
        QFile::remove(QDir::temp().path() % '/'_q % d->socket);
        if (!d->server.listen(d->socket))
            return false;
    }
    connect(&d->server, &QLocalServer::newConnection, [this]() {
        QScopedPointer<QLocalSocket> socket(d->server.nextPendingConnection());
        if (socket) {
            while (socket->bytesAvailable() < (int)sizeof(quint32))
                socket->waitForReadyRead();
            QDataStream in(socket.data());
            QByteArray msg;
            quint32 left;
            in >> left;
            msg.resize(left);
            char *buffer = msg.data();
            do {
                const int read = in.readRawData(buffer, left);
                if (read < 0)
                    return;
                left -= read;
                buffer += read;
            } while (left > 0 && socket->waitForReadyRead(5000));
            socket->write(ack, qstrlen(ack));
            socket->waitForBytesWritten(1000);
            socket->close();
            emit messageReceived(msg); //### (might take a long time to return)
        }
    });
    return true;
}

auto LocalConnection::sendMessage(const QByteArray &msg, int timeout) -> bool
{
    if (runServer())
        return false;
    QLocalSocket socket;
    socket.connectToServer(d->socket);
    if (!socket.waitForConnected(timeout))
        return false;
    QDataStream out(&socket);
    out.writeBytes(msg.constData(), msg.size());
    bool res = socket.waitForBytesWritten(timeout);
    res &= socket.waitForReadyRead(timeout);   // wait for ack
    res &= (socket.read(qstrlen(ack)) == ack);
    return res;
}
