#include "mrl.hpp"
#include "tmp/algorithm.hpp"
#include "misc/udf25.hpp"
#include <QCryptographicHash>

Mrl::Mrl(const QUrl &url) {
    if (url.isLocalFile())
        m_loc = "file://"_a % url.toLocalFile();
    else
        m_loc = url.toString();
}

Mrl::Mrl(const QString &location, const QString &name) {
    if (location.isEmpty())
        return;
    m_loc = location;
    const int idx = location.indexOf("://"_a);
    if (idx < 0)
        m_loc = "file://"_a % _ToAbsFilePath(location);
    else if (startsWith("file://"_a))
        m_loc = QUrl::fromPercentEncoding(location.toUtf8());
    else if (startsWith("dvdnav://"_a) || startsWith("bdnav://"_a) || startsWith("cue://"_a))
        m_loc = location;
    else
        m_loc = QUrl::fromPercentEncoding(location.toUtf8());
    m_name = name;
}

auto Mrl::path() const -> QString
{
    return isLocalFile() ? m_loc : QUrl(m_loc).path();
}

auto Mrl::fileName() const -> QString
{
    const auto path = this->path();
    return path.mid(path.lastIndexOf('/'_q) + 1);
}

auto Mrl::suffix() const -> QString
{
    const auto path = this->path();
    const int idx = path.lastIndexOf('.'_q);
    return idx != -1 ? path.mid(idx + 1) : QString();
}

auto Mrl::displayName() const -> QString
{
    if (!m_name.isEmpty())
        return m_name;
    if (isLocalFile())
        return fileName();
    QString disc;
    if (isDvd())
        disc = qApp->translate("Mrl", "DVD");
    else if (isBluray())
        disc = qApp->translate("Mrl", "Blu-ray");
    if (disc.isEmpty()) {
        const auto suffix = this->suffix();
        if (_IsSuffixOf(MediaExt, suffix))
            return path();
        return m_loc;
    }
    auto dev = device();
    if (dev.isEmpty())
        return disc;
    if (!dev.startsWith("/dev/"_a)) {
        QRegEx regex(u"/([^/]+)/*$"_q);
        auto match = regex.match(dev);
        if (match.hasMatch())
            dev = match.captured(1);
    }
    return disc % " ("_a % dev % ')'_q;
}

auto Mrl::isImage() const -> bool
{
    return _IsSuffixOf(ImageExt, suffix());
}

auto Mrl::isEmpty() const -> bool
{
    const int idx = m_loc.indexOf("://"_a);
    return (idx < 0) || !(idx+3 < m_loc.size());
}

static const QStringList discSchemes = QStringList()
        << u"dvdnav"_q << u"bdnav"_q;

auto Mrl::isDisc() const -> bool
{
    return discSchemes.contains(scheme(), Qt::CaseInsensitive);
}

auto Mrl::device() const -> QString
{
    const auto scheme = this->scheme();
    if (!discSchemes.contains(scheme, Qt::CaseInsensitive))
        return QString();
    auto path = m_loc.midRef(scheme.size() + 3);
    const int idx = path.indexOf('/'_q);
    if (idx < 0)
        return QString();
    return path.mid(idx+1).toString();
}

auto Mrl::fromDisc(const QString &scheme, const QString &device,
                   int title, bool hash) -> Mrl
{
    QString loc = scheme % "://"_a;
    if (title < 0)
        loc += u"menu"_q;
    else if (title >= 0)
        loc += QString::number(title);
    Mrl mrl(loc % '/'_q % device);
    if (hash)
        mrl.updateHash();
    return mrl;
}

auto Mrl::fromCueTrack(const QString &cue, const CueTrack &track,
                       const QString &name) -> Mrl
{
    const QStringList list { "cue://"_a % cue, track.file, _N(track.start), _N(track.end) };
    return Mrl(list.join(u":;"_q), name);
}

auto Mrl::isCueTrack() const -> bool
{
    return m_loc.startsWith("cue://"_a);
}

auto Mrl::cueSheet() const -> QString
{
    if (!isCueTrack())
        return QString();
    const int end = m_loc.indexOf(u":;"_q, 6);
    if (end < 6)
        return QString();
    return m_loc.mid(6, end - 6);
}

auto Mrl::toCueTrack() const -> CueTrack
{
    if (!isCueTrack())
        return CueTrack();
    const auto strs = m_loc.mid(6).split(u":;"_q);
    if (strs.size() != 4)
        return CueTrack();
    CueTrack track;
    track.file = strs[1];
    track.start = strs[2].toInt();
    track.end = strs[3].toInt();
    return track;
}

auto Mrl::titleMrl(int title) const -> Mrl
{
    auto mrl = fromDisc(scheme(), device(), title, false);
    mrl.m_hash = m_hash;
    return mrl;
}

static QByteArray dvdHash(const QString &device) {
    static QStringList files = QStringList()
        << u"/VIDEO_TS/VIDEO_TS.IFO"_q
        << u"/VIDEO_TS/VTS_01_0.IFO"_q
        << u"/VIDEO_TS/VTS_02_0.IFO"_q
        << u"/VIDEO_TS/VTS_03_0.IFO"_q
        << u"/VIDEO_TS/VTS_04_0.IFO"_q
        << u"/VIDEO_TS/VTS_05_0.IFO"_q
        << u"/VIDEO_TS/VTS_06_0.IFO"_q
        << u"/VIDEO_TS/VTS_07_0.IFO"_q
        << u"/VIDEO_TS/VTS_08_0.IFO"_q
        << u"/VIDEO_TS/VTS_09_0.IFO"_q;
    static constexpr int block = 2048;
    QByteArray data;
    if (QFileInfo(device).isDir()) {
        for (auto &fileName : files) {
            QFile file(device % fileName);
            if (!file.open(QFile::ReadOnly))
                break;
            data += file.read(block);
        }
    } else {
        udf::udf25 udf;
        if (!udf.Open(device.toLocal8Bit()))
            return QByteArray();
        for (auto &fileName : files) {
            ::udf::File file(&udf, fileName);
            if (!file.isOpen())
                break;
            data += file.read(block);
        }
    }
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

static QByteArray blurayHash(const QString &device) {
    static constexpr int block = 2048;
    QStringList files = QStringList()
            << u"/BDMV/index.bdmv"_q << u"/BDMV/MovieObject.bdmv"_q;
    QByteArray data;
    if (QFileInfo(device).isDir()) {
        auto dir = [&] (const QString &path) {
            QDir dir(device % path);
            auto list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            const int count = qMin(5, list.size());
            for (int i=0; i<count; ++i)
                files.append(path % '/'_q % list[i]);
        };
        dir(u"/BDMV/PLAYLIST"_q);
        dir(u"/BDMV/CLIPINF"_q);
        dir(u"/BDMV/STREAM"_q);
        tmp::sort(files);
        for (auto &fileName : files) {
            QFile file(device % fileName);
            if (file.open(QFile::ReadOnly))
                data += file.read(block);
        }
    } else {
        udf::udf25 fs;
        if (!fs.Open(device.toLocal8Bit()))
            return QByteArray();
        auto dir = [&] (const QString &path) {
            ::udf::Dir dir(&fs, path);
            const auto list = dir.files();
            const int count = qMin(5, list.size());
            for (int i=0; i<count; ++i)
                files.append(list[i]);
        };
        dir(u"/BDMV/PLAYLIST"_q);
        dir(u"/BDMV/CLIPINF"_q);
        dir(u"/BDMV/STREAM"_q);
        tmp::sort(files);
        for (auto &fileName : files) {
            ::udf::File file(&fs, fileName);
            if (file.isOpen())
                data += file.read(block);
        }
    }
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

auto Mrl::calculateHash(const Mrl &mrl) -> QByteArray
{
    if (!mrl.isDisc())
        return QByteArray();
    const auto device = mrl.device();
    if (device.isEmpty())
        return QByteArray();
    return mrl.isDvd() ? dvdHash(device) : blurayHash(device);
}

auto Mrl::updateHash() -> void
{
    m_hash = calculateHash(*this);
}

auto Mrl::toUnique() const -> Mrl
{
    if (!isDisc())
        return *this;
    if (m_hash.isEmpty())
        return Mrl();
    Mrl mrl;
    mrl.m_loc = scheme() % ":///"_a % QString::fromUtf8(m_hash);
    mrl.m_hash = m_hash;
    mrl.m_name = m_name;
    return mrl;
}

auto Mrl::fromUniqueId(const QString &id, const QString &device, const QString &name) -> Mrl
{
    Mrl mrl;
    mrl.m_loc = id;
    mrl.m_name = name;
    if (!mrl.isDisc())
        return mrl;
    mrl.m_hash = mrl.device().toUtf8();
    mrl.m_loc = mrl.scheme() % "://"_a;
    if (!device.isEmpty())
        mrl.m_loc += '/'_q % device;
    return mrl;
}

auto Mrl::isYouTube() const -> bool
{
    QRegEx rx(uR"(^https?://(www\.)?youtube\.com)"_q, QRegEx::CaseInsensitiveOption);
    return rx.match(m_loc).hasMatch();
}

auto Mrl::isDir() const -> bool
{
    return QFileInfo(toLocalFile()).isDir();
}

auto operator << (QDataStream &lhs, const Mrl &rhs) -> QDataStream&
{
    lhs << (qint32)0 << rhs.toString() << rhs.name();
    return lhs;
}

auto operator >> (QDataStream &lhs, Mrl &rhs) -> QDataStream&
{
    qint32 ver = 0; QString loc, name;
    lhs >> ver >> loc >> name;
    rhs = Mrl(loc, name);
    return lhs;
}
