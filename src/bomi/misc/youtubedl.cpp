#include "youtubedl.hpp"
#include "log.hpp"
#include "dialog/bbox.hpp"
#include "tmp/algorithm.hpp"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QProcess>

DECLARE_LOG_CONTEXT(YouTubeDL)

auto YouTubeFormat::operator < (const YouTubeFormat &rhs) const -> bool
{
    auto &lhs = *this;
    if (lhs.isAudio() != rhs.isAudio())
        return lhs.isAudio();
    if (lhs.isAudio()) {
        if (lhs.tbr() != rhs.tbr())
            return lhs.tbr() > rhs.tbr();
        return lhs.fps() > rhs.fps();
    }
    const auto ls = lhs.width() * lhs.height();
    const auto rs = rhs.width() * rhs.height();
    if (ls != rs)
        return ls > rs;
    if (lhs.fps() != rhs.fps())
        return lhs.fps() > rhs.fps();
    return lhs.tbr() > rhs.tbr();
}

auto YouTubeFormat::isValid() const -> bool
{
    if (m_url.isEmpty() || m_id.isEmpty())
        return false;
    if (m_dash && !m_tbr)
        return false;
    if (!m_audio && (!m_width || !m_height))
        return false;
    return true;
}

auto YouTubeFormat::fromJson(const QJsonObject &json) -> YouTubeFormat
{
    YouTubeFormat format;
    format.m_id = json[u"format_id"_q].toString();
    format.m_url = json[u"url"_q].toString();
    format.m_prefer = json[u"preference"_q].toInt();
    format.m_tbr = json[u"tbr"_q].toInt();
    format.m_ext = json[u"ext"_q].toString();
    const auto note = json[u"format_note"_q].toString();
    const auto audio = note.contains("DASH audio"_a);
    format.m_live = note.contains("HLS"_a);
    format.m_dash = note.contains("DASH "_a);
    format.m_audio = audio;
    format.m_3d = note.contains("3D"_a);
    if (audio) {
        format.m_fps = json[u"asr"_q].toDouble();
        format.m_codec = json[u"acodec"_q].toString();
        format.m_bps = json[u"abr"_q].toInt();
    } else {
        format.m_fps = json[u"fps"_q].toDouble();
        format.m_codec = json[u"vcodec"_q].toString();
        format.m_bps = json[u"vbr"_q].toInt();
        format.m_width = json[u"width"_q].toInt();
        format.m_height = json[u"height"_q].toInt();
        if (format.m_live && !format.m_width)
            format.m_width = format.m_height * 16.0 / 9.0 + 0.5;
    }
    return format;
}

auto YouTubeFormat::isLive() const -> bool
{
    return m_live;
}

/******************************************************************************/

struct YouTubeDL::Data {
    YouTubeDL *p = nullptr;
    QTemporaryDir cookieDir;
    QString userAgent, program = u"youtube-dl"_q;
    QString input;
    Error error = NoError;
    QProcess *proc = nullptr;
    int timeout = 60000;
    QMutex mutex;
    QJsonObject json;
    GetFormatFunc getFormat;
    bool ask = false;
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

auto YouTubeDL::setAskVideoQuality(bool ask) -> void
{
    d->mutex.lock();
    d->ask = ask;
    d->mutex.unlock();
}

auto YouTubeDL::setGetFormat(GetFormatFunc &&func) -> void
{
    d->getFormat = std::move(func);
}

auto YouTubeDL::result() const -> Result
{
    Result r;
    if (d->json[u"_type"_q].toString() == "playlist"_a) {
        const auto entries = d->json[u"entries"_q].toArray();
        if (entries.isEmpty()) {
            _Warn("Empty playlist detected. Do nothing.");
            return Result();
        }
        const auto first = entries[0];
        const auto test = first.toObject()[u"webpage_url"_q];
        if (test.isString() && test.toString() == d->json[u"webpage_url"_q].toString()) {
            r.url = "edl://"_a;
            for (int i = 0; i < entries.size(); ++i)
                r.url += entries[i].toObject()[u"url"_q].toString() % ';'_q;
            r.title = d->json[u"title"_q].toString();
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
        return r;
    }
    r.title = d->json[u"title"_q].toString();

    QList<YouTubeFormat> formats;
    bool live = false;
    for (auto fmt : d->json[u"formats"_q].toArray()) {
        auto format = YouTubeFormat::fromJson(fmt.toObject());
        live |= format.isLive();
        if (format.isValid())
            formats.push_back(format);
    }
    if (live) {
        QList<YouTubeFormat> fmts;
        for (auto &fmt : formats) {
            if (fmt.isLive())
                fmts.push_back(fmt);
        }
        formats = std::move(fmts);
    }
    d->mutex.lock();
    const bool ask = d->ask;
    d->mutex.unlock();
    const auto defId = d->json[u"format_id"_q].toString();
    if (!ask) {
        if (formats.isEmpty()) {
            r.url = d->json[u"url"_q].toString();
            if (r.url.isEmpty()) {
                _Error("No URL found.");
                return Result();
            }
            return r;
        }
        for (auto &fmt : formats) {
            if (fmt.id() == defId) {
                r.url = fmt.url();
                return r;
            }
        }
        qSort(formats);
        r.url = formats.front().url();
        return r;
    }
    if (!d->getFormat)
        return Result();
    d->getFormat(&formats, defId);
    if (formats.isEmpty())
        return Result();
    r.duration = d->json[u"duration"_q].toDouble() * 1000;
    r.url = formats.front().url();
    if (formats.size() > 1)
        r.audio = formats[1].url();
    r.live = formats.front().isLive();
    return r;
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
    d->json = QJsonObject();
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
    d->json = json;
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

struct YouTubeDialog::Data {
    QComboBox *video, *audio;
    QList<YouTubeFormat> formats;
};

YouTubeDialog::YouTubeDialog()
    : d(new Data)
{
    d->video = new QComboBox;
    d->audio = new QComboBox;

    connect(SIGNAL_VT(d->video, currentIndexChanged, int), this, [=] (int idx) {
        if (idx != -1)
            d->audio->setEnabled(d->formats[d->video->currentData().toInt()].isDash());
    });

    auto form = new QFormLayout(this);
    form->addRow(tr("Video"), d->video);
    form->addRow(tr("Audio"), d->audio);
    form->addRow(BBox::make(this));
}

YouTubeDialog::~YouTubeDialog()
{
    delete d;
}

auto YouTubeDialog::setFormats(const QList<YouTubeFormat> &formats, const QString &def) -> void
{
    d->formats = formats;
    d->video->clear();
    d->audio->clear();
    tmp::sort(d->formats, [] (const YouTubeFormat &lhs, const YouTubeFormat &rhs) {
        if (lhs.isAudio() != rhs.isAudio())
            return lhs.isAudio();
        if (lhs.isAudio()) {
            if (lhs.tbr() != rhs.tbr())
                return lhs.tbr() > rhs.tbr();
            return lhs.fps() > rhs.fps();
        }
        const auto ls = lhs.width() * lhs.height();
        const auto rs = rhs.width() * rhs.height();
        if (ls != rs)
            return ls > rs;
        if (lhs.fps() != rhs.fps())
            return lhs.fps() > rhs.fps();
        return lhs.tbr() > rhs.tbr();
    });
    for (int i = 0; i < d->formats.size(); ++i) {
        const auto &f = d->formats[i];
        if (f.is3D())
            continue;
        auto _n = [] (int v) { return v ? _N(v) : u"--"_q; };
        QString desc;
        if (f.isAudio())
            desc = _n(f.fps() + 0.5) % "Hz@"_a % _n(f.bitrate());
        else {
            if (f.isDash())
                desc = '['_q % tr("Video Only") % ']'_q;
            else if (f.isLive())
                desc = '['_q % tr("Live") % ']'_q;
            desc += _n(f.width()) % 'x'_q % _n(f.height()) % ','_q % _n(f.fps() + 0.5) % "fps"_a;
        }
        desc += '('_q % f.codec() % ','_q % f.extension() % ')'_q;
        (f.isAudio() ? d->audio : d->video)->addItem(desc, i);
    }
    for (int i = 0; i < d->video->count(); ++i) {
        if (d->formats[d->video->itemData(i).toInt()].id() == def) {
            d->video->setCurrentIndex(i);
            break;
        }
    }
    d->audio->setEnabled(d->formats[d->video->currentData().toInt()].isDash());
}

auto YouTubeDialog::formats() const -> QList<YouTubeFormat>
{
    if (!d->video->count() || !result())
        return QList<YouTubeFormat>();
    QList<YouTubeFormat> formats;
    formats << d->formats[d->video->currentData().toInt()];
    if (!formats.front().isDash())
        return formats;
    if (!d->audio->count())
        return QList<YouTubeFormat>();
    return formats << d->formats[d->audio->currentData().toInt()];
}
