#include "mrl.hpp"
#include "info.hpp"
#include "misc/udf25.hpp"

Mrl::Mrl(const QUrl &url) {
    if (url.isLocalFile())
        m_loc = _L("file://") % url.toLocalFile();
    else
        m_loc = url.toString();
}

Mrl::Mrl(const QString &location, const QString &name) {
    if (location.isEmpty())
        return;
    const int idx = location.indexOf("://");
    if (idx < 0)
        m_loc = _L("file://") % QFileInfo(location).absoluteFilePath();
    else if (location.startsWith("file://", Qt::CaseInsensitive))
        m_loc = QUrl::fromPercentEncoding(location.toUtf8());
    else if (location.startsWith("dvdnav://", Qt::CaseInsensitive) || location.startsWith("bdnav://", Qt::CaseInsensitive))
        m_loc = location;
    else
        m_loc = QUrl::fromPercentEncoding(location.toUtf8());
    m_name = name;
}

auto Mrl::isPlaylist() const -> bool
{
    return Info::playlistExt().contains(suffix(), Qt::CaseInsensitive);
}

auto Mrl::fileName() const -> QString
{
    const int idx = m_loc.lastIndexOf('/');
    return m_loc.mid(idx + 1);
}

auto Mrl::suffix() const -> QString
{
    const int idx = m_loc.lastIndexOf('.');
    if (idx != -1)
        return m_loc.mid(idx + 1);
    return QString();
}

auto Mrl::displayName() const -> QString
{
    if (isLocalFile())
        return fileName();
    QString disc;
    if (isDvd())
        disc = qApp->translate("Mrl", "DVD");
    else if (isBluray())
        disc = qApp->translate("Mrl", "Blu-ray");
    if (disc.isEmpty())
        return location();
    auto dev = device();
    if (dev.isEmpty())
        return disc;
    if (!dev.startsWith(_L("/dev/"))) {
        QRegularExpression regex("/([^/]+)/*$");
        auto match = regex.match(dev);
        if (match.hasMatch())
            dev = match.captured(1);
    }
    return disc % _L(" (") % dev % _L(')');
}

bool Mrl::isImage() const { return Info::readableImageExt().contains(suffix(), Qt::CaseInsensitive); }

auto Mrl::isEmpty() const -> bool
{
    const int idx = m_loc.indexOf("://");
    return (idx < 0) || !(idx+3 < m_loc.size());
}

static const QStringList discSchemes = QStringList() << _L("dvdnav") << _L("bdnav");

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
    const int idx = path.indexOf(_L('/'));
    if (idx < 0)
        return QString();
    return path.mid(idx+1).toString();
}

auto Mrl::fromDisc(const QString &scheme, const QString &device, int title, bool hash) -> Mrl
{
    QString loc = scheme % _L("://");
    if (title < 0)
        loc += _L("menu");
    else if (title >= 0)
        loc += QString::number(title);
    Mrl mrl(loc % _L('/') % device);
    if (hash)
        mrl.updateHash();
    return mrl;
}

auto Mrl::titleMrl(int title) const -> Mrl
{
    auto mrl = fromDisc(scheme(), device(), title, false);
    mrl.m_hash = m_hash;
    return mrl;
}

static QByteArray dvdHash(const QString &device) {
    static QStringList files = QStringList()
        << _L("/VIDEO_TS/VIDEO_TS.IFO")
        << _L("/VIDEO_TS/VTS_01_0.IFO")
        << _L("/VIDEO_TS/VTS_02_0.IFO")
        << _L("/VIDEO_TS/VTS_03_0.IFO")
        << _L("/VIDEO_TS/VTS_04_0.IFO")
        << _L("/VIDEO_TS/VTS_05_0.IFO")
        << _L("/VIDEO_TS/VTS_06_0.IFO")
        << _L("/VIDEO_TS/VTS_07_0.IFO")
        << _L("/VIDEO_TS/VTS_08_0.IFO")
        << _L("/VIDEO_TS/VTS_09_0.IFO");
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
    QStringList files = QStringList() << _L("/BDMV/index.bdmv")
        << _L("/BDMV/MovieObject.bdmv");
    QByteArray data;
    if (QFileInfo(device).isDir()) {
        auto dir = [&] (const QString &path) {
            QDir dir(device % path);
            auto list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            const int count = qMin(5, list.size());
            for (int i=0; i<count; ++i)
                files.append(path % _L('/') % list[i]);
        };
        dir("/BDMV/PLAYLIST");
        dir("/BDMV/CLIPINF");
        dir("/BDMV/STREAM");
        qSort(files);
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
        dir("/BDMV/PLAYLIST");
        dir("/BDMV/CLIPINF");
        dir("/BDMV/STREAM");
        qSort(files);
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
    mrl.m_loc = scheme() % _L(":///") % QString::fromLatin1(m_hash);
    mrl.m_hash = m_hash;
    mrl.m_name = m_name;
    return mrl;
}

auto Mrl::fromUniqueId(const QString &id, const QString &device) -> Mrl
{
    Mrl mrl;
    mrl.m_loc = id;
    if (!mrl.isDisc())
        return mrl;
    mrl.m_hash = mrl.device().toLatin1();
    mrl.m_loc = mrl.scheme() % _L("://");
    if (!device.isEmpty())
        mrl.m_loc += _L('/') % device;
    return mrl;
}
