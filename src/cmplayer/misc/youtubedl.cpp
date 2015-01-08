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
//    auto mbox(const QString &msg) -> void
//    {
//        if (!running)
//            return;
//        running = false;
//        MBox::critical(p, p->windowTitle(), msg, { BBox::Ok });
//        p->reject();
//        proc.kill();
//        proc.waitForFinished();
//    }
};

YouTubeDL::YouTubeDL(QObject *parent)
    : QObject(parent), d(new Data)
{
    d->p = this;
//    setLabelText(tr("Retrieving YouTube video address..."));
//    setRange(0, 0);

//    using Error = Signal<QProcess, QProcess::ProcessError>;
//    using Finished = Signal<QProcess, int, QProcess::ExitStatus>;
//    connect(&d->proc, static_cast<Error>(&QProcess::error), this, [=] () {
//        switch (d->proc.error()) {
//        case QProcess::FailedToStart:
//            d->mbox(tr("Failed to start '%1'").arg(d->program));
//            break;
//        default:
//            break;
//        }
//    });
//    connect(&d->proc, static_cast<Finished>(&QProcess::finished), this, [=] () {
//        d->videoUrl.clear();
//        if (d->proc.exitStatus() == QProcess::NormalExit && !d->proc.exitCode()) {
//            const auto address = d->proc.readAllStandardOutput().trimmed();
//            if (address.startsWith("http"))
//                d->videoUrl = QUrl::fromPercentEncoding(address);
//        }
//        switch (d->proc.error()) {
//        case QProcess::Crashed:
//            d->mbox(tr("Script crashed"));
//            break;
//        case QProcess::WriteError:
//        case QProcess::ReadError:
//            d->mbox(tr("IO error occurred"));
//            break;
//        default:
//            if (d->videoUrl.isEmpty())
//                d->mbox(tr("Unknown error occurred"));
//            break;
//        }
//        if (!d->videoUrl.isEmpty())
//            accept();
//    });
//    connect(this, &YouTubeDL::canceled, this, [=] () {
//        d->running = false;
//        d->proc.kill();
//        d->videoUrl.clear();
//        reset();
//    });
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
    const auto address = proc.readAllStandardOutput().trimmed();
    d->output = QUrl::fromPercentEncoding(address);
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
