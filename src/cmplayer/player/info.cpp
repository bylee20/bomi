#include "info.hpp"

Info Info::self;

Info::Info() {
    m_privPath = QString::fromLocal8Bit(qgetenv("CMPLAYER_PRIVATE_PATH"));
    if (m_privPath.isEmpty()) {
        QDir dir = QDir::home();
        if (!dir.exists(".cmplayer"))
            dir.mkdir(".cmplayer");
        dir.cd(".cmplayer");
        m_privPath = dir.absolutePath();
    }
    m_videoExt << "3gp" << "3iv"
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
    m_audioExt << "aac" << "ac3" << "aiff" << "m4a" << "mka" << "mp3"
               << "ogg" << "pcm" << "vaw" << "wav" << "waw" << "wma";
    m_subExt   << "ass" << "smi" << "srt" << "ssa" << "sub" << "txt";
    m_plExt    << "pls" << "m3u" << "m3u8";
}

Info::~Info() {}

auto Info::ExtList::toFilter(const QString &name) const -> QString
{
    if (isEmpty())
        return QString();
    QString filter;
    if (!name.isEmpty())
        filter += name % _L(' ');
    filter += _L('(');
    for (auto &ext : *this)
        filter += _L("*.") % ext % _L(' ');
    filter[filter.size() - 1] = _L(')');
    return filter;
}

auto Info::ExtList::toNameFilter() const -> QStringList
{
    QStringList nameFilter;
    for (QStringList::const_iterator it = begin(); it != end(); ++it)
        nameFilter << ("*." + *it);
    return nameFilter;
}

auto Info::pluginPath() -> const char*
{
    return "";
}

static auto convert(const QList<QByteArray> &formats) -> Info::ExtList
{
    Info::ExtList exts;
    for (auto &format : formats)
        exts << QString::fromLocal8Bit(format);
    return exts;
}

auto Info::readableImageExt() -> const Info::ExtList&
{
    if (self.m_rImgExt.isEmpty())
        self.m_rImgExt = convert(QImageReader::supportedImageFormats());
    return self.m_rImgExt;
}

auto Info::writableImageExt() -> const Info::ExtList&
{
    if (self.m_wImgExt.isEmpty())
        self.m_wImgExt = convert(QImageWriter::supportedImageFormats());
    return self.m_wImgExt;
}
