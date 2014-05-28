#ifndef OPENGLLOGGER_HPP
#define OPENGLLOGGER_HPP

class OpenGLLogger : public QObject {
    Q_OBJECT
public:
    OpenGLLogger(const QByteArray &category, QObject *parent = nullptr);
    ~OpenGLLogger();
    auto initialize(QOpenGLContext *ctx) -> bool;
    auto finalize(QOpenGLContext *ctx) -> void;
    static auto isAvailable() -> bool;
private:
    struct Data;
    Data *d;
};

#endif // OPENGLLOGGER_HPP
