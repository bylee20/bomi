#include "yledl.hpp"
#include <QProcess>
#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#endif

struct YleDL::Data {
    YleDL *p = nullptr;
    Error error = NoError;
    QString input;
    QString program = u"yle-dl"_q;
    QString fifo;
    QProcess *proc = nullptr;
    bool quit = false;
    QMutex mutex;
    auto close() {
    }
};

YleDL::YleDL(QObject *parent)
    : QThread(parent), d(new Data)
{
    d->p = this;
    d->fifo = QDir::tempPath() % u"/bomi-yle-"_q % QString::number(qApp->applicationPid());
#ifdef Q_OS_LINUX
    mkfifo(d->fifo.toLocal8Bit(), 0666);
#endif
}

YleDL::~YleDL()
{
    cancel();
    QFile::remove(d->fifo);
    delete d;
}

auto YleDL::supports(const QString &url) const -> bool
{
#ifdef Q_OS_LINUX
    return url.startsWith(u"http://areena.yle.fi"_q, Qt::CaseInsensitive);
#else
    Q_UNUSED(url);
    return false;
#endif
}

auto YleDL::setProgram(const QString &program) -> void
{
    d->program = program;
}

auto YleDL::program() const -> QString
{
    return d->program;
}

auto YleDL::run(const QString &url) -> bool
{
    d->close();
    d->input = url;
    d->error = Unsupported;
    if (!supports(url))
        return false;
    d->error = NoError;
    start();
    return true;
}

auto YleDL::error() const -> Error
{
    return d->error;
}

auto YleDL::url() const -> QString
{
    return d->fifo;
}

auto YleDL::cancel() -> void
{
    d->mutex.lock();
    if (d->proc)
        d->proc->terminate();
    d->mutex.unlock();
    d->mutex.lock();
    if (d->proc)
        d->proc->kill();
    d->mutex.unlock();
    wait(1000);
    terminate();
    wait(600000);
}

auto YleDL::run() -> void
{
    QProcess proc;
    proc.setStandardOutputFile(d->fifo, QIODevice::WriteOnly);
    QStringList args;
    args << u"--pipe"_q;;

    d->mutex.lock();
    d->proc = &proc;
    args << d->input;
    d->mutex.unlock();

    proc.start(d->program, args);
    proc.waitForFinished(-1);

    d->mutex.lock();
    d->proc = nullptr;
    d->mutex.unlock();
}
