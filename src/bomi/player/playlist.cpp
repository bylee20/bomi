#include "playlist.hpp"
#include "tmp/algorithm.hpp"

Playlist::Playlist()
: QList<Mrl>() {}

Playlist::Playlist(const Playlist &rhs)
: QList<Mrl>(rhs) {}

Playlist::Playlist(const Mrl &mrl): QList<Mrl>() {
    push_back(mrl);
}

Playlist::Playlist(const Mrl &mrl, const QString &enc) {
    load(mrl, enc);
}

Playlist::Playlist(const QList<Mrl> &rhs)
: QList<Mrl>(rhs) {}

auto Playlist::sort() -> void
{
    QCollator c;
    c.setNumericMode(true);

    tmp::sort(*this, [&](Mrl a, Mrl b) -> bool
        { return c.compare(a.fileName(), b.fileName()) < 0; });
}

auto Playlist::save(const QString &filePath, Type type) const -> bool
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;
    if (type == Unknown)
        type = guessType(file.fileName());
    QTextStream out(&file);
    out.setCodec("UTF-8");
    switch (type) {
    case PLS:
        return savePLS(out);
    case M3U:
    case M3U8:
        return saveM3U(out);
    default:
        return false;
    }
}

auto Playlist::load(QTextStream &in, QString enc, Type type,
                    const QUrl &url) -> bool
{
    clear();
    if (type == M3U8)
        enc = u"UTF-8"_q;
    if (!enc.isEmpty())
        in.setCodec(QTextCodec::codecForName(enc.toLocal8Bit()));
    switch (type) {
    case PLS:
        return loadPLS(in, url);
    case M3U:
    case M3U8:
        return loadM3U(in, url);
    default:
        return false;
    }
}

auto Playlist::load(const QUrl &url, QByteArray *data,
                    const QString &enc, Type type) -> bool
{
    QTextStream in(data);
    return load(in, enc, type, url);
}

auto Playlist::load(const QString &filePath, const QString &enc, Type type) -> bool
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return false;
    if (type == Unknown)
        type = guessType(filePath);
    QTextStream in(&file);
    return load(in, enc, type);
}

auto Playlist::load(const Mrl &mrl, const QString &enc, Type type) -> bool
{
    if (mrl.isLocalFile())
        return load(mrl.toLocalFile(), enc, type);
    return false;
}

auto Playlist::guessType(const QString &fileName) -> Playlist::Type
{
    const auto suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix == "pls"_a)
        return PLS;
    else if (suffix == "m3u"_a)
        return M3U;
    else if (suffix == "m3u8"_a)
        return M3U8;
    else
        return Unknown;
}

auto Playlist::savePLS(QTextStream &out) const -> bool
{
    const int count = size();
    out << "[playlist]" << endl << "NumberOfEntries=" << count << endl << endl;
    for (int i=0; i<count; ++i)
        out << "File" << i+1 << '=' << at(i).toString() << endl
                << "Length" << i+1 << '=' << -1 << endl << endl;
    out << "Version=2" << endl;
    return true;
}

auto Playlist::saveM3U(QTextStream &out) const -> bool
{
    const int count = size();
    out << "#EXTM3U\n";
    for (int i=0; i<count; ++i) {
        out << "#EXTINF:" << 0 << ',' << "" << '\n';
        out << at(i).toString() << '\n';
    }
    return true;
}


auto Playlist::loadPLS(QTextStream &in, const QUrl &url) -> bool
{
    const qint64 pos = in.pos();
    in.seek(0);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty())
            continue;
        static QRegEx rxFile(uR"(^File\d+=(.+)$)"_q);
        const auto match = rxFile.match(line);
        if (match.hasMatch())
            push_back(Mrl(resolve(match.captured(1), url)));
    }
    in.seek(pos);
    return true;
}

auto Playlist::loadM3U(QTextStream &in, const QUrl &url) -> bool
{
    const qint64 pos = in.pos();
    in.seek(0);
    auto getNextLocation = [&in] () -> QString {
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'_q))
                return line;
        }
        return QString();
    };

    QRegEx rxExtInf(uR"(#EXTINF\s*:\s*(?<num>(-|)\d+)\s*\,\s*(?<name>.*)\s*$)"_q);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QString name, location;
        if (line.startsWith('#'_q)) {
            auto matched = rxExtInf.match(line);
            if (matched.hasMatch()) {
                name = matched.captured(u"name"_q);
                location = getNextLocation();
            }
        } else
            location = line;
        if (!location.isEmpty())
            push_back(Mrl(resolve(location, url), name));
    }
    in.seek(pos);
    return true;
}

auto Playlist::resolve(const QString &location, const QUrl &url) -> QString
{
    if (url.isEmpty() || location.indexOf("://"_a) > 0)
        return location;
    const QFileInfo info(location);
    if (info.isAbsolute())
        return location;
    const auto str = url.toString();
    const auto idx = str.lastIndexOf('/'_q);
    if (idx < 0)
        return location;
    return str.left(idx + 1) % location;
}

auto Playlist::save(const QString &name, QSettings *set) const -> void
{
    set->beginWriteArray(name, size());
    for (int i=0; i<size(); ++i) {
        set->setArrayIndex(i);
        set->setValue(u"mrl"_q, at(i).toString());
        set->setValue(u"name"_q, at(i).name());
    }
    set->endArray();
}

auto Playlist::load(const QString &name, QSettings *set) -> void
{
    clear();
    const int size = set->beginReadArray(name);
    for (int i=0; i<size; ++i) {
        set->setArrayIndex(i);
        const Mrl mrl(set->value(u"mrl"_q).toString(),
                      set->value(u"name"_q).toString());
        if (!mrl.isEmpty())
            push_back(mrl);
    }
    set->endArray();
}
