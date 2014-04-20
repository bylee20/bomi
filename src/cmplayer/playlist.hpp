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
    void save(const QString &prefix, QSettings *set) const;
    void load(const QString &prefix, QSettings *set);
    bool save(const QString &filePath, Type type = Unknown) const;
    bool load(const QString &filePath, const QString &enc = QString(), Type type = Unknown);
    bool load(const Mrl &url, const QString &enc = QString(), Type type = Unknown);
    bool load(QByteArray *data, const QString &enc, Type type);
    Playlist &loadAll(const QDir &dir);
    static Type guessType(const QString &fileName);
private:
    bool savePLS(QTextStream &out) const;
    bool saveM3U(QTextStream &out) const;
    bool load(QTextStream &in, QString enc, Type type);
    bool loadPLS(QTextStream &in);
    bool loadM3U(QTextStream &in);
};

#endif // PLAYLIST_HPP
