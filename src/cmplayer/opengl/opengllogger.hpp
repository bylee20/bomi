#ifndef OPENGLLOGGER_HPP
#define OPENGLLOGGER_HPP

class OpenGLLogger : public QObject {
    Q_OBJECT
public:
    OpenGLLogger(const QByteArray &category, QObject *parent = nullptr);
    ~OpenGLLogger();
    auto initialize(QOpenGLContext *ctx, bool autolog = true) -> bool;
    auto finalize(QOpenGLContext *ctx) -> void;
    auto print(const QOpenGLDebugMessage &message) -> void;
    static auto isAvailable() -> bool;
private:
    struct Data;
    Data *d;
};

#endif // OPENGLLOGGER_HPP
