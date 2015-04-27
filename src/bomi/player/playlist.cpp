#include "playlist.hpp"
#include "misc/encodinginfo.hpp"
#include "tmp/algorithm.hpp"
#include "misc/objectstorage.hpp"
#include <QCollator>
#include <QTextStream>
#include <QTextCodec>

Playlist::Playlist()
: QList<Mrl>() {}

Playlist::Playlist(const Playlist &rhs)
: QList<Mrl>(rhs) {}

Playlist::Playlist(const Mrl &mrl): QList<Mrl>() {
    push_back(mrl);
}

Playlist::Playlist(const Mrl &mrl, const EncodingInfo &enc) {
    load(mrl, enc);
}

Playlist::Playlist(const QList<Mrl> &rhs)
: QList<Mrl>(rhs) {}

auto Playlist::sort() -> void
{
    QCollator c;
    c.setNumericMode(true);

    tmp::sort(*this, [&](Mrl a, Mrl b) -> bool
        { return c.compare(a.toString(), b.toString()) < 0; });
}

auto Playlist::save(const QString &filePath, Type type) const -> bool
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;
    if (type == Unknown)
        type = guessType(file.fileName());
    QTextStream out(&file);
    out.setCodec(EncodingInfo::utf8().codec());
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

auto Playlist::load(QTextStream &in, const EncodingInfo &_enc,
                    Type type, const QUrl &url) -> bool
{
    clear();
    EncodingInfo enc = _enc;
    if (type == M3U8)
        enc = EncodingInfo::utf8();
    if (enc.isValid())
        in.setCodec(enc.codec());
    const qint64 pos = in.pos();
    in.seek(0);
    auto ret = [&] () {
        switch (type) {
        case PLS:
            return loadPLS(in, url);
        case M3U:
        case M3U8:
            return loadM3U(in, url);
        case Cue:
            return loadCue(in, url);
        default:
            return false;
        }
    }();
    in.seek(pos);
    return ret;
}

auto Playlist::load(const QUrl &url, QByteArray *data,
                    const EncodingInfo &enc, Type type) -> bool
{
    QTextStream in(data);
    return load(in, enc, type, url);
}

auto Playlist::load(const QString &filePath, const EncodingInfo &enc, Type type) -> bool
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return false;
    if (type == Unknown)
        type = guessType(filePath);
    QTextStream in(&file);
    return load(in, enc, type, _UrlFromLocalFile(filePath));
}

auto Playlist::load(const Mrl &mrl, const EncodingInfo &enc, Type type) -> bool
{
    if (mrl.isLocalFile())
        return load(mrl.toLocalFile(), enc, type);
    return false;
}

auto Playlist::typeForSuffix(const QString &str) -> Type
{
    const auto suffix = str.toLower();
    if (suffix == "cue"_a)
        return Cue;
    if (suffix == "pls"_a)
        return PLS;
    if (suffix == "m3u"_a)
        return M3U;
    if (suffix == "m3u8"_a)
        return M3U8;
    return Unknown;
}

auto Playlist::guessType(const QString &fileName) -> Playlist::Type
{
    return typeForSuffix(QFileInfo(fileName).suffix());
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
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty())
            continue;
        static QRegEx rxFile(uR"(^File\d+=(.+)$)"_q);
        const auto match = rxFile.match(line);
        if (match.hasMatch())
            push_back(Mrl(resolve(match.captured(1), url)));
    }
    return true;
}

auto Playlist::loadM3U(QTextStream &in, const QUrl &url) -> bool
{
    auto getNextLocation = [&in] () -> QString {
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'_q))
                return line;
        }
        return QString();
    };

    QRegEx rxExtInf(uR"(#EXTINF\s*:\s*(?<num>(-|)\d+)[^,]*,\s*(?<name>.*)\s*$)"_q);
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
    return true;
}

struct CueTrack {
    QString title, writer, performer, file;
    int idx00 = -1, idx01 = -1;
    auto toMrl(const QString &cue, const CueTrack *next) const -> Mrl
    {
        Mrl::CueTrack track;
        track.file = file;
        track.start = idx01;
        if (next)
            track.end = next->idx00 != -1 ? next->idx00 : next->idx01;
        QString name;
        if (!title.isEmpty())
            name += title;
        if (!performer.isEmpty()) {
            if (!name.isEmpty())
                name += " - "_a;
            name += performer;
        }
        return Mrl::fromCueTrack(cue, track, name);
    }
};

auto Playlist::loadCue(QTextStream &in, const QUrl &url) -> bool
{
    CueTrack init;
    CueTrack *current = &init;
    QRegEx rxField(uR"#(^(\w+)\s+(.*)\s*$)#"_q);
    QRegEx rxText(uR"#(^\s*"(.*)"\s*$)#"_q);
    QRegEx rxFile(uR"#(^\s*"(.*)"\s+\w+\s*$)#"_q);
    QRegEx rxIndex(uR"(^\s*(\d\d)\s+(\d\d):(\d\d):(\d\d)\s*$)"_q);
    QVector<CueTrack> tracks;
    while (!in.atEnd()) {
        const auto line = in.readLine().trimmed();
        auto m = rxField.match(line);
        if (!m.hasMatch())
            continue;
        const auto key = m.captured(1);
        if (key == "REM"_a)
            continue;
        const auto value = m.captured(2);
        if (key == "TRACK"_a) {
            tracks.push_back(*current);
            current = &tracks.last();
            current->idx00 = current->idx01 = -1;
            continue;
        }
        if (key == "FILE"_a) {
            m = rxFile.match(value);
            if (!m.hasMatch())
                return false;
            current->file = resolve(m.captured(1), url);
            continue;
        }
        if (key == "INDEX"_a) {
            m = rxIndex.match(value);
            if (!m.hasMatch())
                return false;
            const auto idx = m.capturedRef(1).toInt();
            const auto min = m.capturedRef(2).toInt();
            const auto sec = m.capturedRef(3).toInt()
                    + m.capturedRef(4).toInt() / 75.0;
            const auto msec = (min * 60 + sec) * 1000;
            if (idx == 1)
                current->idx01 = msec;
            else if (idx == 0)
                current->idx00 = msec;
            continue;
        }
#define TEST_TEXT(name, var) \
        if (key == name) { \
            m = rxText.match(value); \
            if (!m.hasMatch()) \
                return false; \
            var = m.captured(1); \
            continue; \
        }
        TEST_TEXT("TITLE"_a, current->title);
        TEST_TEXT("PERFORMER"_a, current->performer);
        TEST_TEXT("SONGWRITER"_a, current->writer);
#undef TEST_TEXT
    }

    const auto cue = url.toLocalFile();
    reserve(tracks.size());
    for (int i = 0; i < tracks.size(); ++i) {
        const auto next = i + 1 < tracks.size() ? &tracks[i+1] : nullptr;
        push_back(tracks[i].toMrl(cue, next));
    }
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

auto operator << (QDataStream &out, const Playlist &pl) -> QDataStream&
{
    return out << static_cast<const QList<Mrl>&>(pl);
}

auto operator >> (QDataStream &in, Playlist &pl) -> QDataStream&
{
    return in >> static_cast<QList<Mrl>&>(pl);
}
