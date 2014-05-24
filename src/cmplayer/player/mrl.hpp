#ifndef MRL_HPP
#define MRL_HPP

#include "stdafx.hpp"

class Mrl {
public:
    Mrl() {}
    Mrl(const QUrl &url);
    Mrl(const QString &location, const QString &name = QString());
    bool operator == (const Mrl &rhs) const {return m_loc == rhs.m_loc;}
    bool operator != (const Mrl &rhs) const {return !(*this == rhs);}
    bool operator < (const Mrl &rhs) const {return m_loc < rhs.m_loc;}
    auto location() const -> QString { auto loc = toLocalFile(); return loc.isEmpty() ? m_loc : loc; }
    auto toString() const -> QString { return m_loc; }
    auto isLocalFile() const -> bool {return m_loc.startsWith(_L("file://"), Qt::CaseInsensitive);}
    auto isDvd() const -> bool {return m_loc.startsWith(_L("dvdnav://"), Qt::CaseInsensitive);}
    auto isBluray() const -> bool { return m_loc.startsWith(_L("bdnav://"), Qt::CaseInsensitive); }
    auto isDisc() const -> bool;
    auto scheme() const -> QString {return m_loc.left(m_loc.indexOf(_L("://")));}
    auto toLocalFile() const -> QString {return isLocalFile() ? m_loc.right(m_loc.size() - 7) : QString();}
    auto fileName() const -> QString;
    auto isPlaylist() const -> bool;
    auto displayName() const -> QString;
    auto isEmpty() const -> bool;
    auto suffix() const -> QString;
    auto name() const -> QString { return m_name; }
    auto isImage() const -> bool;
    static auto fromString(const QString str) -> Mrl { Mrl mrl; mrl.m_loc = str; return mrl; }
    static auto fromDisc(const QString &scheme, const QString &device, int title, bool hash) -> Mrl;
    auto titleMrl(int title) const -> Mrl;
    auto device() const -> QString;
    auto toLocal8Bit() const -> QByteArray { return m_loc.toLocal8Bit(); }
    auto hash() const -> QByteArray { return m_hash; }
    auto updateHash() -> void;
    static auto calculateHash(const Mrl &mrl) -> QByteArray;
    auto isUnique() const -> bool { return !isDisc() || !m_hash.isEmpty(); }
    auto toUnique() const -> Mrl;
    static Mrl fromUniqueId(const QString &id, const QString &device = QString());
private:
    QString m_loc = {};
    QString m_name;
    QByteArray m_hash;
};

Q_DECLARE_METATYPE(Mrl)

#endif // MRL_HPP
