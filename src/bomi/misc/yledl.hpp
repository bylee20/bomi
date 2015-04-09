#ifndef YLEDL_HPP
#define YLEDL_HPP


class YleDL : public QThread {
public:
    enum Error {
        NoError, Canceled, FailedToStart, Timeout, Crashed,
        ReadError, UnknownError, Unsupported
    };
    YleDL(QObject *parent = nullptr);
    ~YleDL();
    auto setProgram(const QString &program) -> void;
    auto program() const -> QString;
    auto run(const QString &url) -> bool;
    auto error() const -> Error;
    auto url() const -> QString;
    auto cancel() -> void;
    auto supports(const QString &url) const -> bool;
private:
    auto run() -> void override;
    struct Data;
    Data *d;
};

#endif // YLEDL_HPP
