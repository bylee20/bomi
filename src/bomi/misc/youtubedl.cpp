#include "youtubedl.hpp"
#include "log.hpp"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QProcess>

DECLARE_LOG_CONTEXT(YouTubeDL)

struct YouTubeDL::Data {
    YouTubeDL *p = nullptr;
    QTemporaryDir cookieDir;
    QString userAgent, program = u"youtube-dl"_q;
    QString input;
    Error error = NoError;
    QProcess *proc = nullptr;
    int timeout = 60000;
    QMutex mutex;
    Result result;
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

auto YouTubeDL::result() const -> Result
{
    return d->result;
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
    d->result = Result();
    d->error = NoError;
    QFile(cookies()).remove();
    QStringList args;
    if (!d->userAgent.isEmpty())
        args << u"--user-agent"_q << d->userAgent;
    args << u"--cookies"_q << cookies();
    args << u"--flat-playlist"_q << u"--no-playlist"_q << u"--all-subs"_q;
    args << u"--sub-format"_q
         << (url.contains("crunchyroll.com"_a, Qt::CaseInsensitive) ? u"ass"_q : u"srt"_q);
    args << u"-J"_q << d->input;

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
    if (d->error) {
        auto out = proc.readAllStandardOutput().trimmed();
        if (!out.isEmpty())
            _Warn(out);
        auto err = proc.readAllStandardError().trimmed();
        if (!err.isEmpty())
            _Error(err);
        return false;
    }
    auto out = proc.readAllStandardOutput().trimmed();
    const auto json =_JsonFromString(_L(std::move(out)));

    // ported from mpv/player/lua/ytdl_hook.lua
    if (json[u"direct"_q].toBool())
        return false;
    auto &r = d->result;
    if (json[u"_type"_q].toString() == "playlist"_a) {
        const auto entries = json[u"entries"_q].toArray();
        if (entries.isEmpty()) {
            _Warn("Empty playlist detected. Do nothing.");
            return false;
        }
        const auto first = entries[0];
        const auto test = first.toObject()[u"webpage_url"_q];
        if (test.isString() && test.toString() == json[u"webpage_url"_q].toString()) {
            r.url = "edl://"_a;
            for (int i = 0; i < entries.size(); ++i)
                r.url += entries[i].toObject()[u"url"_q].toString() % ';'_q;
            r.title = json[u"title"_q].toString();
        } else {
            for (int i = 0; i < entries.size(); ++i) {
                Mrl mrl;
                const auto entry = entries[i].toObject();
                auto it = entry.find(u"webpage_url"_q);
                if (it != entry.end())
                    r.playlist.push_back(Mrl(it.value().toString()));
                else
                    r.playlist.push_back(Mrl(entry[u"url"_q].toString()));
            }
        }
    } else {
        r.url = json[u"url"_q].toString();
        if (r.url.isEmpty()) {
            _Error("No URL found.");
            return false;
        }
        r.title = json[u"title"_q].toString();
        // handle subtitles
    }
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

auto YouTubeDL::supports(const QString &url) -> bool
{
    QProcess proc;
    QStringList args;
    args << u"-g"_q << url;

//    proc.start(d->program, args, QProcess::ReadOnly);
    proc.start(u"youtube-dl"_q, args, QProcess::ReadOnly);
    if (!proc.waitForFinished(30000))
        proc.kill();
    if (proc.error() != QProcess::UnknownError)
        return false;
    if (proc.exitStatus() != QProcess::NormalExit)
        return false;
    if (proc.exitCode())
        return false;
    const auto out = proc.readAllStandardOutput().trimmed();
    const auto err = proc.readAllStandardError().trimmed();
    return !out.isEmpty() && err.isEmpty();
}
