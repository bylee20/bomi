#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include "stdafx.hpp"
#include "mrl.hpp"

class QFile;        class QDir;

class Playlist : public QList<Mrl> {
public:
    enum Type {Unknown, PLS, M3U, M3U8};
    Playlist();
    Playlist(const Playlist &rhs);
    Playlist(const Mrl &mrl);
    Playlist(const QList<Mrl> &rhs);
    Playlist(const Mrl &mrl, const QString &enc);
    auto save(const QString &prefix, QSettings *set) const -> void;
    auto load(const QString &prefix, QSettings *set) -> void;
    auto save(const QString &filePath, Type type = Unknown) const -> bool;
    bool load(const QString &filePath, const QString &enc = QString(), Type type = Unknown);
    bool load(const Mrl &url, const QString &enc = QString(), Type type = Unknown);
    auto load(QByteArray *data, const QString &enc, Type type) -> bool;
    Playlist &loadAll(const QDir &dir);
    static auto guessType(const QString &fileName) -> Type;
private:
    auto savePLS(QTextStream &out) const -> bool;
    auto saveM3U(QTextStream &out) const -> bool;
    auto load(QTextStream &in, QString enc, Type type) -> bool;
    auto loadPLS(QTextStream &in) -> bool;
    auto loadM3U(QTextStream &in) -> bool;
};

#endif // PLAYLIST_HPP
