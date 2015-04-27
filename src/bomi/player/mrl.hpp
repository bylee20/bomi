#ifndef MRL_HPP
#define MRL_HPP

#include "global.hpp"

class Mrl {
public:
    struct CueTrack { QString file; int start = -1, end = -1; };
    Mrl() {}
    Mrl(const QUrl &url);
    Mrl(const QString &location, const QString &name = QString());
    auto operator == (const Mrl &rhs) const -> bool {return m_loc == rhs.m_loc;}
    auto operator != (const Mrl &rhs) const -> bool {return !(*this == rhs);}
    auto operator < (const Mrl &rhs) const -> bool {return m_loc < rhs.m_loc;}
    auto location() const -> QString
        { auto loc = toLocalFile(); return loc.isEmpty() ? m_loc : loc; }
    auto toString() const -> QString { return m_loc; }
    auto startsWith(const QString &s) const -> bool
        { return m_loc.startsWith(s, Qt::CaseInsensitive); }
    auto startsWith(const QLatin1String &s) const -> bool
        { return m_loc.startsWith(s, Qt::CaseInsensitive); }
    auto isLocalFile() const -> bool { return startsWith("file://"_a); }
    auto isDvd() const -> bool { return startsWith("dvdnav://"_a); }
    auto isBluray() const -> bool { return startsWith("bdnav://"_a); }
    auto isDisc() const -> bool;
    auto isCueTrack() const -> bool;
    auto isRemoteUrl() const -> bool { return !isLocalFile() && !isDisc(); }
    auto scheme() const -> QString {return m_loc.left(m_loc.indexOf("://"_a));}
    auto toLocalFile() const -> QString
        {return isLocalFile() ? m_loc.right(m_loc.size() - 7) : QString();}
    auto fileName() const -> QString;
//    auto isPlaylist() const -> bool;
    auto displayName() const -> QString;
    auto isEmpty() const -> bool;
    auto isYouTube() const -> bool;
    auto suffix() const -> QString;
    auto name() const -> QString { return m_name; }
    auto isImage() const -> bool;
    auto titleMrl(int title) const -> Mrl;
    auto device() const -> QString;
    auto toLocal8Bit() const -> QByteArray { return m_loc.toLocal8Bit(); }
    auto toUtf8() const -> QByteArray { return m_loc.toUtf8(); }
    auto hash() const -> QByteArray { return m_hash; }
    auto updateHash() -> void;
    auto isUnique() const -> bool { return !isDisc() || !m_hash.isEmpty(); }
    auto toUnique() const -> Mrl;
    auto isDir() const -> bool;
    auto start() const -> int;
    auto end() const -> int;
    auto toCueTrack() const -> CueTrack;
    auto cueSheet() const -> QString;
    static auto fromString(const QString str) -> Mrl
        { Mrl mrl; mrl.m_loc = str; return mrl; }
    static auto fromDisc(const QString &scheme, const QString &device,
                         int title, bool hash) -> Mrl;
    static auto fromCueTrack(const QString &cue, const CueTrack &track,
                             const QString &name) -> Mrl;
    static auto calculateHash(const Mrl &mrl) -> QByteArray;
    static auto fromUniqueId(const QString &id,
                             const QString &device = QString(),
                             const QString &name = QString()) -> Mrl;
private:
    auto path() const -> QString;
    QString m_loc = {};
    QString m_name;
    QByteArray m_hash;
};

Q_DECLARE_METATYPE(Mrl)

auto operator << (QDataStream &lhs, const Mrl &rhs) -> QDataStream&;
auto operator >> (QDataStream &lhs, Mrl &rhs) -> QDataStream&;

#endif // MRL_HPP
