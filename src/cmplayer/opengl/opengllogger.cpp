#include "opengllogger.hpp"
#include "openglmisc.hpp"
#include "misc/log.hpp"
#ifdef Q_OS_LINUX
#include <GL/glx.h>
#endif

struct OpenGLLogger::Data {
    QOpenGLDebugLogger *logger = nullptr;
    QByteArray category;
    template<class... Args>
    auto error(const QByteArray &format, const Args&... args) -> void
    {
        if (Log::maximumLevel() >= Log::Error)
            Log::write(category, Log::Error, format, args...);
    }
    template<class... Args>
    auto debug(const QByteArray &format, const Args&... args) -> void
    {
        if (Log::maximumLevel() >= Log::Debug)
            Log::write(category, Log::Debug, format, args...);
    }
};

OpenGLLogger::OpenGLLogger(const QByteArray &category, QObject *parent)
    : QObject(parent), d(new Data)
{
    d->category = "OpenGL/" + category;
}

OpenGLLogger::~OpenGLLogger()
{
    delete d;
}

SIA _ToLog(QOpenGLDebugMessage::Source source) -> QByteArray
{
    switch (source) {
#define SWITCH_SOURCE(s) case QOpenGLDebugMessage::s##Source: return #s;
    SWITCH_SOURCE(API)                SWITCH_SOURCE(Invalid)
    SWITCH_SOURCE(WindowSystem)        SWITCH_SOURCE(ShaderCompiler)
    SWITCH_SOURCE(ThirdParty)        SWITCH_SOURCE(Application)
    SWITCH_SOURCE(Other)            SWITCH_SOURCE(Any)
#undef SWITCH_SOURCE
    }
    return QByteArray::number(source, 16);
}

SIA _ToLog(QOpenGLDebugMessage::Type type) -> QByteArray
{
    switch (type) {
#define SWITCH_TYPE(t) case QOpenGLDebugMessage::t##Type: return #t;
    SWITCH_TYPE(Invalid)            SWITCH_TYPE(Error)
    SWITCH_TYPE(DeprecatedBehavior)    SWITCH_TYPE(UndefinedBehavior)
    SWITCH_TYPE(Portability)        SWITCH_TYPE(Performance)
    SWITCH_TYPE(Other)                SWITCH_TYPE(Marker)
    SWITCH_TYPE(GroupPush)            SWITCH_TYPE(GroupPop)
    SWITCH_TYPE(Any)
#undef SWITCH_TYPE
    }
    return QByteArray::number(type, 16);
}

SIA _ToLog(QOpenGLDebugMessage::Severity severity) -> QByteArray
{
    switch (severity) {
#define SWITCH_SEVERITY(s) case QOpenGLDebugMessage::s##Severity: return #s;
    SWITCH_SEVERITY(Invalid)        SWITCH_SEVERITY(High)
    SWITCH_SEVERITY(Medium)            SWITCH_SEVERITY(Low)
    SWITCH_SEVERITY(Notification)    SWITCH_SEVERITY(Any)
#undef SWITCH_SEVERITY
    }
    return QByteArray::number(severity, 16);
}

auto OpenGLLogger::isAvailable() -> bool
{
    return OGL::hasExtension(OGL::Debug);
}

auto OpenGLLogger::finalize(QOpenGLContext */*ctx*/) -> void
{
    if (d->logger && d->logger->isLogging())
        d->logger->stopLogging();
    _Delete(d->logger);
}

auto OpenGLLogger::print(const QOpenGLDebugMessage &message) -> void
{
    if (message.type() == QOpenGLDebugMessage::ErrorType)
        d->error("Error: %%", message.message().trimmed());
    else
        d->debug("Logger: %% (%%/%%/%%)", message.message().trimmed(),
                 message.source(), message.severity(), message.type());
}

auto OpenGLLogger::initialize(QOpenGLContext *ctx, bool autolog) -> bool
{
    if (!ctx->format().testOption(QSurfaceFormat::DebugContext)) {
        d->error("OpenGL debug logger was not requested.");
        return false;
    }
    if (!OGL::hasExtension(OGL::Debug)) {
        d->error("OpenGL debug logger is not supported.");
        return false;
    }
    d->logger = new QOpenGLDebugLogger;
    if (!d->logger->initialize()) {
        OGL::logError("OpenGLCompat::initialize()"_b);
        _Delete(d->logger);
        return false;
    }
    d->debug("OpenGL debug logger is running.");

    d->logger->disableMessages();
    QOpenGLDebugMessage::Types types = QOpenGLDebugMessage::InvalidType;
    switch (Log::maximumLevel()) {
    case Log::Trace:
        types = QOpenGLDebugMessage::AnyType;
        break;
    case Log::Debug:
        types |= QOpenGLDebugMessage::PerformanceType
                | QOpenGLDebugMessage::MarkerType;
    case Log::Info:
    case Log::Warn:
        types |= QOpenGLDebugMessage::DeprecatedBehaviorType
                | QOpenGLDebugMessage::PortabilityType;
    case Log::Error:
    case Log::Fatal:
        types |= QOpenGLDebugMessage::ErrorType
                | QOpenGLDebugMessage::UndefinedBehaviorType;
    }
    d->logger->enableMessages(QOpenGLDebugMessage::AnySource, types);

    if (autolog)
        connect(d->logger, &QOpenGLDebugLogger::messageLogged,
                this, &OpenGLLogger::print, Qt::DirectConnection);
    connect(d->logger, &QOpenGLDebugLogger::messageLogged,
            this, &OpenGLLogger::logged, Qt::DirectConnection);
#ifdef CMPLAYER_RELEASE
    d->logger->startLogging(QOpenGLDebugLogger::AsynchronousLogging);
#else
    d->logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
#endif
    return true;
}
