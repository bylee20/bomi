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
        QStringList toNameFilter() const;
    };
    ~Info();
    static const QString &privatePath() {return self.m_privPath;}
    static const ExtList &readableImageExt();
    static const ExtList &writableImageExt();
    static const ExtList &videoExt() {return self.m_videoExt;}
    static const ExtList &audioExt() {return self.m_audioExt;}
    static const ExtList &subtitleExt() {return self.m_subExt;}
    static const ExtList &playlistExt() {return self.m_plExt;}
    static QString readableImageExtFilter(const QString &name = tr("Images")) { return readableImageExt().toFilter(name); }
    static QString writableImageExtFilter(const QString &name = tr("Images")) { return writableImageExt().toFilter(name); }
    static QString videoExtFilter(const QString &name = tr("Video Files")) {return self.m_videoExt.toFilter(name);}
    static QString audioExtFilter(const QString &name = tr("Audio Files")) {return self.m_audioExt.toFilter(name);}
    static QString subtitleExtFilter(const QString &name = tr("Subtitle Files")) {return self.m_subExt.toFilter(name);}
    static QString playlistExtFilter(const QString &name = tr("Playlist Files")) {return self.m_plExt.toFilter(name);}
    static QStringList videoNameFilter() {return self.m_videoExt.toNameFilter();}
    static QStringList audioNameFilter() {return self.m_audioExt.toNameFilter();}
    static QStringList subtitleNameFilter() {return self.m_subExt.toNameFilter();}
    static QStringList playlistNameFilter() {return self.m_plExt.toNameFilter();}
    static constexpr int versionNumber() { return 0x00814; }
    static constexpr const char *version() {return "0.8.14";}
    static constexpr const char *name() {return "CMPlayer";}
    static QString mediaExtFilter() {
        return videoExtFilter() % _L(";;") % audioExtFilter() % _L(";;")
            % readableImageExtFilter() % ";;" % tr("All Files") % _L(' ') % _L("(*.*)");
    }
    static QStringList mediaNameFilter() {return videoNameFilter() + audioNameFilter() + readableImageExt().toNameFilter();}
    static const char *pluginPath();
private:
    Info();
    ExtList m_videoExt, m_audioExt, m_subExt, m_plExt, m_rImgExt, m_wImgExt;
    QString m_privPath;
    static Info self;
};

#endif // INFO_HPP
