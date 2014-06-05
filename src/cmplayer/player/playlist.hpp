#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

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
    auto load(const QUrl &url, QByteArray *data, const QString &enc, Type type) -> bool;
    Playlist &loadAll(const QDir &dir);
    static auto guessType(const QString &fileName) -> Type;
private:
    static auto resolve(const QString &location, const QUrl &url) -> QString;
    auto savePLS(QTextStream &out) const -> bool;
    auto saveM3U(QTextStream &out) const -> bool;
    auto load(QTextStream &in, QString enc, Type type,
              const QUrl &url = QUrl()) -> bool;
    auto loadPLS(QTextStream &in, const QUrl &url = QUrl()) -> bool;
    auto loadM3U(QTextStream &in, const QUrl &url = QUrl()) -> bool;
};

#endif // PLAYLIST_HPP
