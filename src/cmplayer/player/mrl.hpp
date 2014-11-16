#ifndef MRL_HPP
#define MRL_HPP

class Mrl {
public:
    Mrl() {}
    Mrl(const QUrl &url);
    Mrl(const QString &location, const QString &name = QString());
    auto operator == (const Mrl &rhs) const -> bool {return m_loc == rhs.m_loc;}
    auto operator != (const Mrl &rhs) const -> bool {return !(*this == rhs);}
    auto operator < (const Mrl &rhs) const -> bool {return m_loc < rhs.m_loc;}
    auto location() const -> QString
        { auto loc = toLocalFile(); return loc.isEmpty() ? m_loc : loc; }
    auto toString() const -> QString { return m_loc; }
    auto isLocalFile() const -> bool {return m_loc.startsWith("file://"_a, QCI);}
    auto isDvd() const -> bool {return m_loc.startsWith("dvdnav://"_a, QCI);}
    auto isBluray() const -> bool { return m_loc.startsWith("bdnav://"_a, QCI); }
    auto isDisc() const -> bool;
    auto scheme() const -> QString {return m_loc.left(m_loc.indexOf("://"_a));}
    auto toLocalFile() const -> QString {return isLocalFile() ? m_loc.right(m_loc.size() - 7) : QString();}
    auto fileName() const -> QString;
//    auto isPlaylist() const -> bool;
    auto displayName() const -> QString;
    auto isEmpty() const -> bool;
    auto suffix() const -> QString;
    auto name() const -> QString { return m_name; }
    auto isImage() const -> bool;
    auto titleMrl(int title) const -> Mrl;
    auto device() const -> QString;
    auto toLocal8Bit() const -> QByteArray { return m_loc.toLocal8Bit(); }
    auto hash() const -> QByteArray { return m_hash; }
    auto updateHash() -> void;
    auto isUnique() const -> bool { return !isDisc() || !m_hash.isEmpty(); }
    auto toUnique() const -> Mrl;
    auto isDir() const { return QFileInfo(toLocalFile()).isDir(); }
    static auto fromString(const QString str) -> Mrl
        { Mrl mrl; mrl.m_loc = str; return mrl; }
    static auto fromDisc(const QString &scheme, const QString &device,
                         int title, bool hash) -> Mrl;
    static auto calculateHash(const Mrl &mrl) -> QByteArray;
    static auto fromUniqueId(const QString &id,
                             const QString &device = QString()) -> Mrl;
private:
    auto path() const -> QString;
    QString m_loc = {};
    QString m_name;
    QByteArray m_hash;
};

Q_DECLARE_METATYPE(Mrl)

#endif // MRL_HPP
