#ifndef YOUTUBEDL_HPP
#define YOUTUBEDL_HPP

#include <QObject>

class YouTubeDL : public QObject
{
    Q_OBJECT
public:
    enum Error {
        NoError, Canceled, FailedToStart, Timeout, Crashed,
        ReadError, UnknownError, Unsupported
    };
    YouTubeDL(QObject *parent = nullptr);
    ~YouTubeDL();
    auto userAgent() const -> QString;
    auto setUserAgent(const QString &ua) -> void;
    auto setProgram(const QString &program) -> void;
    auto program() const -> QString;
    auto setTimeout(int timeout) -> void;
    auto timeout() const -> int;
    auto cookies() const -> QString;
    auto run(const QString &url) -> bool;
    auto error() const -> Error;
    auto url() const -> QString;
    auto cancel() -> void;
private:
    struct Data;
    Data *d;
};

#endif // YOUTUBEDL_HPP
