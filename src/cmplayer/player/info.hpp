#ifndef INFO_HPP
#define INFO_HPP

#include "stdafx.hpp"

class Info {
    Q_DECLARE_TR_FUNCTIONS(Info)
public:
    class ExtList : public QStringList {
    public:
        ExtList() {}
        ExtList(const QStringList &other): QStringList(other) {}
        ExtList(const ExtList &other): QStringList(other) {}
        QString toFilter(const QString &name = QString()) const;
        auto toNameFilter() const -> QStringList;
    };
    ~Info();
    static constexpr auto versionNumber() -> int { return 0x00814; }
    static constexpr auto version() -> const char* { return "0.8.14"; }
    static constexpr auto name() -> const char* { return "CMPlayer"; }
    static auto privatePath() -> const QString& {return self.m_privPath;}
    static auto readableImageExt() -> const ExtList&;
    static auto writableImageExt() -> const ExtList&;
    static auto videoExt() -> const ExtList& {return self.m_videoExt;}
    static auto audioExt() -> const ExtList& {return self.m_audioExt;}
    static auto subtitleExt() -> const ExtList& {return self.m_subExt;}
    static auto playlistExt() -> const ExtList& {return self.m_plExt;}
    static auto readableImageExtFilter(const QString &name = tr("Images"))
        -> QString { return readableImageExt().toFilter(name); }
    static auto writableImageExtFilter(const QString &name = tr("Images"))
        -> QString { return writableImageExt().toFilter(name); }
    static auto videoExtFilter(const QString &name = tr("Video Files"))
        -> QString { return self.m_videoExt.toFilter(name); }
    static auto audioExtFilter(const QString &name = tr("Audio Files"))
        -> QString { return self.m_audioExt.toFilter(name); }
    static auto subtitleExtFilter(const QString &name = tr("Subtitle Files"))
        -> QString { return self.m_subExt.toFilter(name); }
    static auto playlistExtFilter(const QString &name = tr("Playlist Files"))
        -> QString { return self.m_plExt.toFilter(name); }
    static auto videoNameFilter() -> QStringList
        { return self.m_videoExt.toNameFilter(); }
    static auto audioNameFilter() -> QStringList
        { return self.m_audioExt.toNameFilter(); }
    static auto subtitleNameFilter() -> QStringList
        { return self.m_subExt.toNameFilter(); }
    static auto playlistNameFilter() -> QStringList
        { return self.m_plExt.toNameFilter(); }
    static auto mediaExtFilter() -> QString;
    static auto mediaNameFilter() -> QStringList;
    static auto pluginPath() -> const char*;
private:
    Info();
    ExtList m_videoExt, m_audioExt, m_subExt, m_plExt, m_rImgExt, m_wImgExt;
    QString m_privPath;
    static Info self;
};

inline auto Info::mediaExtFilter() -> QString
{
    return videoExtFilter() % _L(";;") % audioExtFilter() % _L(";;")
           % readableImageExtFilter() % _L(";;")
           % tr("All Files") % _L(' ') % _L("(*.*)");
}

inline auto Info::mediaNameFilter() -> QStringList
{
    return videoNameFilter() + audioNameFilter()
           + readableImageExt().toNameFilter();
}

#endif // INFO_HPP
