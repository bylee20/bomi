#include "stdafx.hpp"
#include <zlib.h>

namespace Pch {

static const QStringList videoExts = QStringList()
        << "3gp" << "3iv"
        << "asf" << "avi"
        << "bdm" << "bdmv"
        << "clpi" << "cpk" << "cpi"
        << "dat" << "divx" << "dv"
        << "flac" << "fli" << "flv"
        << "h264"
        << "i263"
        << "m2t" << "m2ts" << "m4v" << "mkv" << "mov" << "mp2"
        << "mp4" << "mpeg" << "mpg" << "mpg2" << "mpg4" << "mpl"
        << "mpls" << "mts"
        << "nsv" << "nut" << "nuv"
        << "ogg" <<"ogm"
        << "qt"
        << "rm" << "rmvb"
        << "trp" << "tp" << "ts"
        << "vcd" << "vfw" << "vob"
        << "wmv";
static const QStringList audioExts = QStringList()
        << "aac" << "ac3" << "aiff" << "m4a" << "mka" << "mp3"
        << "ogg" << "pcm" << "vaw" << "wav" << "waw" << "wma";
static const QStringList subExts = QStringList()
        << "ass" << "smi" << "srt" << "ssa" << "sub" << "txt";
static const QStringList plExts = QStringList() << "pls" << "m3u" << "m3u8";
static const QStringList discExts = QStringList() << "iso";
static const QStringList imageExts = QStringList()
        << "png" << "jpg" << "jpeg" << "gif";

static QMap<QString, QString> lastFolders;
auto open_folders() -> QMap<QString, QString> { return lastFolders; }
auto set_open_folders(const QMap<QString, QString> &folders) -> void
{ lastFolders = folders; }

auto _ExtList(ExtType ext) -> QStringList
{
    switch(ext) {
    case VideoExt:
        return videoExts;
    case AudioExt:
        return audioExts;
    case SubtitleExt:
        return subExts;
    case ImageExt:
        return imageExts;
    case PlaylistExt:
        return plExts;
    case DiscExt:
        return discExts;
    default:
        return QStringList();
    }
}

auto _IsSuffixOf(ExtType ext, const QString &suffix) -> bool
{
    static constexpr auto cs = Qt::CaseInsensitive;
    return _ExtList(ext).contains(suffix, cs);
}

auto _ToNameFilter(ExtTypes exts) -> QStringList
{
    if (!exts)
        return QStringList();
    auto conv = [] (const QStringList &exts)
    {
        QStringList filter;
        filter.reserve(exts.size());
        for (auto &ext : exts)
            filter.push_back("*." + ext);
        return filter;
    };
    QStringList filter;
    if (exts & VideoExt)
        filter += conv(videoExts);
    if (exts & AudioExt)
        filter += conv(audioExts);
    if (exts & SubtitleExt)
        filter += conv(subExts);
    if (exts & ImageExt)
        filter += conv(imageExts);
    if (exts & DiscExt)
        filter += conv(discExts);
    if (exts & PlaylistExt)
        filter += conv(plExts);
    return filter;
}

auto _ToFilter(ExtTypes exts) -> QString
{
    if (!exts)
        return QString();
    auto conv = [] (const QStringList &exts, const QString &name)
    {
        if (exts.isEmpty())
            return QString();
        QString filter;
        if (!name.isEmpty())
            filter += name % _L(' ');
        filter += _L('(');
        for (auto &ext : exts)
            filter += _L("*.") % ext % _L(' ');
        filter[filter.size() - 1] = _L(')');
        return filter;
    };
    QStringList filter;
    if (exts & VideoExt)
        filter += conv(videoExts, qApp->translate("Info", "Video Files"));
    if (exts & AudioExt)
        filter += conv(audioExts, qApp->translate("Info", "Audio Files"));
    if (exts & SubtitleExt)
        filter += conv(subExts, qApp->translate("Info", "Subtitle Files"));
    if (exts & ImageExt)
        filter += conv(imageExts, qApp->translate("Info", "Images"));
    if (exts & DiscExt)
        filter += conv(discExts, qApp->translate("Info", "ISO Image Files"));
    if (exts & PlaylistExt)
        filter += conv(discExts, qApp->translate("Info", "Playlist Files"));
    return filter.join(";;");
}

auto _SetLastOpenFolder(const QString &path, const QString &key) -> void
{
    auto &folder = lastFolders[key];
    if (path.isEmpty())
        folder.clear();
    else
        folder = QFileInfo(path).absolutePath();
}

auto _LastOpenFolder(const QString &key) -> QString
{
    return lastFolders.value(key);
}

auto _GetOpenFileNames(QWidget *parent, const QString &title, ExtTypes exts,
                       const QString &key) -> QStringList
{
    auto &folder = lastFolders[key];
    const auto list = QFileDialog::getOpenFileNames(parent, title, folder,
                                                    _ToFilter(exts));
    if (list.isEmpty())
        return QStringList();
    folder = QFileInfo(list.first()).absolutePath();
    return list;
}

auto _GetOpenFileName(QWidget *parent, const QString &title, ExtTypes exts,
                      const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    const auto file = QFileDialog::getOpenFileName(parent, title, folder,
                                                   _ToFilter(exts));
    if (file.isEmpty())
        return QString();
    folder = QFileInfo(file).absolutePath();
    return file;
}

auto _GetSaveFileName(QWidget *parent, const QString &title,
                      const QString &fileName, ExtTypes exts,
                      const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    QString path = folder, filter;
    if (folder.isEmpty())
        path = fileName;
    else
        path = folder % '/' % fileName;
    auto file = QFileDialog::getSaveFileName(parent, title, path,
                                                   _ToFilter(exts), &filter);
    if (file.isEmpty())
        return QString();
    if (!file.endsWith(filter))
        file += _L('.') % filter;
    folder = QFileInfo(file).absolutePath();
    return file;
}

auto _GetOpenDir(QWidget *parent, const QString &title,
                 const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    const auto dir = QFileDialog::getExistingDirectory(parent, title, folder);
    if (!dir.isEmpty())
        folder = QDir(dir).absolutePath();
    return dir;
}

//auto _GetOpenFileName(QWidget *parent, const QString &title,
//                     const QString &dir, const QString &f) -> QString
//{ return QFileDialog::getOpenFileName(p, t, dir, f, 0); }

//auto _GetSaveFileName(QWidget *p, const QString &t,
//                     const QString &dir, const QString &f) -> QString
//{ return QFileDialog::getSaveFileName(p, t, dir, f, 0); }

//SIA _GetOpenFileName(QWidget *p, const QString &t,
//                     const QString &dir, const QString &f) -> QString
//{ return QFileDialog::getOpenFileName(p, t, dir, f, 0); }

//SIA _GetSaveFileName(QWidget *p, const QString &t,
//                     const QString &dir, const QString &f) -> QString
//{ return QFileDialog::getSaveFileName(p, t, dir, f, 0); }

QByteArray _Uncompress(const QByteArray &data) {
    if (data.size() <= 4)
        return QByteArray();
    QByteArray result;
    int ret;
    z_stream strm;
    static constexpr int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit2(&strm, 15 +  32); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            return QByteArray();
        }
        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}

}
