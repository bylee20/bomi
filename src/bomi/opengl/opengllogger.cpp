#include "opengllogger.hpp"
#include "openglmisc.hpp"
#include "misc/log.hpp"
#include "configure.hpp"
#include <QOpenGLDebugLogger>

static const QHash<QOpenGLDebugMessage::Type, Log::Level> lvGL = [] () {
    QHash<QOpenGLDebugMessage::Type, Log::Level> ret;
    ret[QOpenGLDebugMessage::InvalidType]            = Log::Off;
    ret[QOpenGLDebugMessage::ErrorType]              = Log::Error;
    ret[QOpenGLDebugMessage::DeprecatedBehaviorType] = Log::Warn;
    ret[QOpenGLDebugMessage::PortabilityType]        = Log::Warn;
    ret[QOpenGLDebugMessage::PerformanceType]        = Log::Debug;
    ret[QOpenGLDebugMessage::MarkerType]             = Log::Debug;
    ret[QOpenGLDebugMessage::OtherType]              = Log::Trace;
    ret[QOpenGLDebugMessage::GroupPopType]           = Log::Trace;
    ret[QOpenGLDebugMessage::GroupPushType]          = Log::Trace;
    return ret;
}();

struct OpenGLLogger::Data {
    QOpenGLDebugLogger *logger = nullptr;
    QByteArray category;
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
    SWITCH_SOURCE(API)              SWITCH_SOURCE(Invalid)
    SWITCH_SOURCE(WindowSystem)     SWITCH_SOURCE(ShaderCompiler)
    SWITCH_SOURCE(ThirdParty)       SWITCH_SOURCE(Application)
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
    SWITCH_TYPE(DeprecatedBehavior) SWITCH_TYPE(UndefinedBehavior)
    SWITCH_TYPE(Portability)        SWITCH_TYPE(Performance)
    SWITCH_TYPE(Other)              SWITCH_TYPE(Marker)
    SWITCH_TYPE(GroupPush)          SWITCH_TYPE(GroupPop)
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
    SWITCH_SEVERITY(Medium)         SWITCH_SEVERITY(Low)
    SWITCH_SEVERITY(Notification)   SWITCH_SEVERITY(Any)
#undef SWITCH_SEVERITY
    }
    return QByteArray::number(severity, 16);
}

auto OpenGLLogger::getLogContext() const -> const char*
{
    return d->category.constData();
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
    _WriteLog(lvGL[message.type()], "%% (%%/%%/%%)", message.message().trimmed(),
              message.source(), message.severity(), message.type());
}

auto OpenGLLogger::initialize(QOpenGLContext *ctx, bool autolog) -> bool
{
    if (!ctx->format().testOption(QSurfaceFormat::DebugContext)) {
        _Error("OpenGL debug logger was not requested.");
        return false;
    }
    if (!OGL::hasExtension(OGL::Debug)) {
        _Error("OpenGL debug logger is not supported.");
        return false;
    }
    d->logger = new QOpenGLDebugLogger;
    if (!d->logger->initialize()) {
        OGL::logError("OpenGLCompat::initialize()"_b);
        _Delete(d->logger);
        return false;
    }
    _Debug("OpenGL debug logger is running.");

    d->logger->disableMessages();
    QOpenGLDebugMessage::Types types = QOpenGLDebugMessage::InvalidType;
    for (auto it = lvGL.begin(); it != lvGL.end(); ++it) {
        if (it.value() <= Log::maximumLevel())
            types |= it.key();
    }
    d->logger->enableMessages(QOpenGLDebugMessage::AnySource, types);

    if (autolog)
        connect(d->logger, &QOpenGLDebugLogger::messageLogged,
                this, &OpenGLLogger::print, Qt::DirectConnection);
    connect(d->logger, &QOpenGLDebugLogger::messageLogged,
            this, &OpenGLLogger::logged, Qt::DirectConnection);
#if BOMI_RELEASE
    d->logger->startLogging(QOpenGLDebugLogger::AsynchronousLogging);
#else
    d->logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
#endif
    return true;
}
