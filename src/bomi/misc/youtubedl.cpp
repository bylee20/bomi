#include "youtubedl.hpp"

struct YouTubeDL::Data {
    YouTubeDL *p = nullptr;
    QTemporaryDir cookieDir;
    QString userAgent, program = u"youtube-dl"_q;
    QString input, output;
    Error error = NoError;
    QProcess *proc = nullptr;
    int timeout = 60000;
    QMutex mutex;
};

YouTubeDL::YouTubeDL(QObject *parent)
    : QObject(parent), d(new Data)
{
    d->p = this;
}

YouTubeDL::~YouTubeDL()
{
    delete d;
}

auto YouTubeDL::url() const -> QString
{
    return d->output;
}

auto YouTubeDL::cookies() const -> QString
{
    return d->cookieDir.path() % "/cookies"_a;
}

auto YouTubeDL::userAgent() const -> QString
{
    return d->userAgent;
}

auto YouTubeDL::setUserAgent(const QString &ua) -> void
{
    d->userAgent = ua;
}

auto YouTubeDL::cancel() -> void
{
    QMutexLocker locker(&d->mutex);
    if (!d->proc)
        return;
    if (!d->error)
        d->error = Canceled;
    d->proc->kill();
}

auto YouTubeDL::run(const QString &url) -> bool
{
    QProcess proc;
    d->input = url;
    d->output.clear();
    d->error = NoError;
    QFile(cookies()).remove();
    QStringList args;
    if (!d->userAgent.isEmpty())
        args << u"--user-agent"_q << d->userAgent;
    args << u"--cookies"_q << cookies();
    args << u"--get-url"_q << d->input;
    d->mutex.lock();
    d->proc = &proc;
    d->mutex.unlock();
    proc.start(d->program, args, QProcess::ReadOnly);
    if (!proc.waitForFinished(d->timeout))
        proc.kill();
    d->mutex.lock();
    d->proc = nullptr;
    d->mutex.unlock();
    if (!d->error && proc.error() != QProcess::UnknownError) {
        auto translateError = [&] () {
            switch (proc.error()) {
            case QProcess::FailedToStart:
                return FailedToStart;
            case QProcess::Crashed:
                return Crashed;
            case QProcess::ReadError:
                return ReadError;
            case QProcess::UnknownError:
                return NoError;
            default:
                return UnknownError;
            }
        };
        d->error = translateError();
    }
    if (!d->error && (proc.exitStatus() != QProcess::NormalExit || proc.exitCode()))
        d->error = UnknownError;
    if (d->error)
        return false;
    d->output = QString::fromLocal8Bit(proc.readAllStandardOutput().trimmed());
    return true;
}

auto YouTubeDL::program() const -> QString
{
    return d->program;
}

auto YouTubeDL::setProgram(const QString &program) -> void
{
    d->program = program;
}
