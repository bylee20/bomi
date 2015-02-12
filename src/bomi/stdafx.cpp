#include <zlib.h>
#include <QFileDialog>

namespace Pch {

static const QStringList videoExts = QStringList()
        << u"3gp"_q << u"3iv"_q
        << u"asf"_q << u"avi"_q
        << u"bdm"_q << u"bdmv"_q
        << u"clpi"_q << u"cpk"_q << u"cpi"_q
        << u"dat"_q << u"divx"_q << u"dv"_q
        << u"flac"_q << u"fli"_q << u"flv"_q
        << u"h264"_q
        << u"i263"_q << u"ifo"_q
        << u"m2t"_q << u"m2ts"_q << u"m4v"_q << u"mkv"_q << u"mov"_q
        << u"mp2"_q << u"mp4"_q << u"mpeg"_q << u"mpg"_q << u"mpg2"_q
        << u"mpg4"_q << u"mpl"_q << u"mpls"_q << u"mts"_q
        << u"nsv"_q << u"nut"_q << u"nuv"_q
        << u"ogg"_q << u"ogm"_q
        << u"qt"_q
        << u"rm"_q << u"rmvb"_q
        << u"trp"_q << u"tp"_q << u"ts"_q
        << u"vcd"_q << u"vfw"_q << u"vob"_q
        << u"webm"_q << u"wmv"_q;
static const QStringList audioExts = QStringList()
        << u"aac"_q << u"ac3"_q << u"aiff"_q
        << u"m4a"_q << u"mka"_q << u"mp3"_q
        << u"ogg"_q << u"pcm"_q << u"vaw"_q
        << u"wav"_q << u"waw"_q << u"wma"_q;
static const QStringList subExts = QStringList()
        << u"ass"_q << u"smi"_q << u"srt"_q << u"ssa"_q << u"sub"_q << u"txt"_q;
static const QStringList plExts = QStringList()
        << u"pls"_q << u"m3u"_q << u"m3u8"_q;
static const QStringList discExts = QStringList() << u"iso"_q;
static const QStringList imageExts = QStringList()
        << u"bmp"_q << u"gif"_q << u"jpeg"_q
        << u"jpg"_q << u"png"_q << u"tif"_q << u"tiff"_q;

QStringList writableImageExts;

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
    case WritableImageExt:
        return writableImageExts;
    case PlaylistExt:
        return plExts;
    case DiscExt:
        return discExts;
    default:
        return QStringList();
    }
}

auto _ExtList(ExtTypes exts) -> QStringList
{
    QStringList list;
    if (exts & VideoExt)
        list.append(videoExts);
    if (exts & AudioExt)
        list.append(audioExts);
    if (exts & SubtitleExt)
        list.append(subExts);
    if (exts & ImageExt)
        list.append(imageExts);
    if (exts & WritableImageExt)
        list.append(writableImageExts);
    if (exts & PlaylistExt)
        list.append(plExts);
    if (exts & DiscExt)
        list.append(discExts);
    return list;
}

auto _IsSuffixOf(ExtType ext, const QString &suffix) -> bool
{
    static constexpr auto cs = Qt::CaseInsensitive;
    return _ExtList(ext).contains(suffix, cs);
}

auto _ToAbsPath(const QString &fileName) -> QString
{
    if (fileName.isEmpty())
        return QString();
    return QFileInfo(fileName).absolutePath();
}

auto _ToAbsFilePath(const QString &fileName) -> QString
{
    if (fileName.isEmpty())
        return QString();
    return QFileInfo(fileName).absoluteFilePath();
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
            filter.push_back("*."_a % ext);
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
    if (exts & WritableImageExt)
        filter += conv(writableImageExts);
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
            filter += name % ' '_q;
        filter += '('_q;
        for (auto &ext : exts)
            filter += "*."_a % ext % ' '_q;
        filter[filter.size() - 1] = ')'_q;
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
    if (exts & WritableImageExt)
        filter += conv(writableImageExts, qApp->translate("Info", "Images"));
    if (exts & DiscExt)
        filter += conv(discExts, qApp->translate("Info", "ISO Image Files"));
    if (exts & PlaylistExt)
        filter += conv(discExts, qApp->translate("Info", "Playlist Files"));
    return filter.join(u";;"_q);
}

auto _SetLastOpenPath(const QString &path, const QString &key) -> void
{
    auto &folder = lastFolders[key];
    if (path.isEmpty())
        folder.clear();
    else
        folder = QFileInfo(path).absolutePath();
}

auto _LastOpenPath(const QString &key) -> QString
{
    return lastFolders.value(key);
}

auto _GetOpenFiles(QWidget *parent, const QString &title, ExtTypes exts,
                       const QString &key) -> QStringList
{
    auto &folder = lastFolders[key];
    const auto list = QFileDialog::getOpenFileNames(parent, title, folder,
                                                    _ToFilter(exts));
    if (list.isEmpty())
        return QStringList();
    folder = _ToAbsPath(list.first());
    return list;
}

auto _GetOpenFile(QWidget *parent, const QString &title, ExtTypes exts,
                      const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    const auto file = QFileDialog::getOpenFileName(parent, title, folder,
                                                   _ToFilter(exts));
    if (file.isEmpty())
        return QString();
    folder = _ToAbsPath(file);
    return file;
}

auto _GetSaveFile(QWidget *parent, const QString &title,
                      const QString &fileName, ExtTypes exts,
                      const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    QString path = folder;
    if (folder.isEmpty())
        path = fileName;
    else
        path = folder % '/'_q % fileName;
    QFileDialog dlg(parent, title, path, _ToFilter(exts));
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setConfirmOverwrite(true);
    dlg.setDefaultSuffix(_ExtList(exts).front());
    if (!dlg.exec())
        return QString();
    const auto file = dlg.selectedFiles().value(0);
    if (file.isEmpty())
        return QString();
    folder = QFileInfo(file).absolutePath();
    return file;
}

auto _GetOpenDir(QWidget *parent, const QString &title,
                 const QString &key) -> QString
{
    auto &folder = lastFolders[key];
    const auto dir = QFileDialog::getExistingDirectory(parent, title, folder);
    if (dir.isEmpty())
        return dir;
    const auto ret = QDir(dir).absolutePath();
    folder = ret % '/'_q;
    return ret;
}

auto _WritablePath(Location loc) -> QString
{
    const auto std = static_cast<QStandardPaths::StandardLocation>(loc);
    auto path = QStandardPaths::writableLocation(std);
    if (loc == Location::Config)
        path += '/'_q % qApp->organizationName()
              % '/'_q % qApp->applicationName();
    if (!QDir().mkpath(path))
        return QString();
    return path;
}

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
