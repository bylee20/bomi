#include "youtubedialog.hpp"
#include "mbox.hpp"

struct YouTubeDialog::Data {
    YouTubeDialog *p = nullptr;
    QTemporaryDir cookieDir;
    QString userAgent, program = u"youtube-dl"_q, videoUrl;
    QProcess proc;
    int timeout = 60000;
    QUrl youtube;
    bool running = false;
    auto mbox(const QString &msg) -> void
    {
        if (!running)
            return;
        running = false;
        MBox::critical(p, p->windowTitle(), msg, { BBox::Ok });
        p->reject();
        proc.kill();
        proc.waitForFinished();
    }
};

YouTubeDialog::YouTubeDialog(QWidget *parent)
    : QProgressDialog(parent), d(new Data)
{
    d->p = this;
    setWindowTitle(u"youtube-dl"_q);
    setLabelText(tr("Retrieving YouTube video address..."));
    setRange(0, 0);

    using Error = Signal<QProcess, QProcess::ProcessError>;
    using Finished = Signal<QProcess, int, QProcess::ExitStatus>;
    connect(&d->proc, static_cast<Error>(&QProcess::error), this, [=] () {
        switch (d->proc.error()) {
        case QProcess::FailedToStart:
            d->mbox(tr("Failed to start '%1'").arg(d->program));
            break;
        default:
            break;
        }
    });
    connect(&d->proc, static_cast<Finished>(&QProcess::finished), this, [=] () {
        d->videoUrl.clear();
        if (d->proc.exitStatus() == QProcess::NormalExit && !d->proc.exitCode()) {
            const auto address = d->proc.readAllStandardOutput().trimmed();
            if (address.startsWith("http"))
                d->videoUrl = QUrl::fromPercentEncoding(address);
        }
        switch (d->proc.error()) {
        case QProcess::Crashed:
            d->mbox(tr("Script crashed"));
            break;
        case QProcess::WriteError:
        case QProcess::ReadError:
            d->mbox(tr("IO error occurred"));
            break;
        default:
            if (d->videoUrl.isEmpty())
                d->mbox(tr("Unknown error occurred"));
            break;
        }
        if (!d->videoUrl.isEmpty())
            accept();
    });
    connect(this, &YouTubeDialog::canceled, this, [=] () {
        d->running = false;
        d->proc.kill();
        d->videoUrl.clear();
        reset();
    });
}

YouTubeDialog::~YouTubeDialog()
{
    delete d;
}

auto YouTubeDialog::cookies() const -> QString
{
    return d->cookieDir.path() % "/cookies"_a;
}

auto YouTubeDialog::videoUrl() const -> QString
{
    return d->videoUrl;
}

auto YouTubeDialog::userAgent() const -> QString
{
    return d->userAgent;
}

auto YouTubeDialog::setUserAgent(const QString &ua) -> void
{
    d->userAgent = ua;
}

auto YouTubeDialog::translate(const QUrl &url) -> void
{
    reset();
    d->youtube = url;
    d->videoUrl.clear();
    QFile(cookies()).remove();
    QStringList args;
    if (!d->userAgent.isEmpty())
        args << u"--user-agent"_q << d->userAgent;
    args << u"--cookies"_q << cookies();
    args << u"--get-url"_q << url.toString();
    d->running = true;
    d->proc.start(d->program, args, QProcess::ReadOnly);
}

auto YouTubeDialog::exec() -> int
{
    return QProgressDialog::exec();
}

auto YouTubeDialog::program() const -> QString
{
    return d->program;
}

auto YouTubeDialog::setProgram(const QString &program) -> void
{
    d->program = program;
}
