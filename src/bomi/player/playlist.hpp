#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include "mrl.hpp"
#include "misc/encodinginfo.hpp"

class QFile;                            class QDir;
class EncodingInfo;                     class ObjectStorage;

class Playlist : public QList<Mrl> {
public:
    enum Type {Unknown, PLS, M3U, M3U8, Cue};
    Playlist();
    Playlist(const Playlist &rhs);
    Playlist(const Mrl &mrl);
    Playlist(const QList<Mrl> &rhs);
    Playlist(const Mrl &mrl, const EncodingInfo &enc);
    auto sort() -> void;
    auto save(const QString &prefix, ObjectStorage *set) const -> void;
    auto load(const QString &prefix, ObjectStorage *set) -> void;
    auto save(const QString &filePath, Type type = Unknown) const -> bool;
    bool load(const QString &filePath, const EncodingInfo &enc = EncodingInfo(), Type type = Unknown);
    bool load(const Mrl &url, const EncodingInfo &enc = EncodingInfo(), Type type = Unknown);
    auto load(const QUrl &url, QByteArray *data, const EncodingInfo &enc, Type type) -> bool;
    static auto guessType(const QString &fileName) -> Type;
    static auto typeForSuffix(const QString &suffix) -> Type;
private:
    static auto resolve(const QString &location, const QUrl &url) -> QString;
    auto savePLS(QTextStream &out) const -> bool;
    auto saveM3U(QTextStream &out) const -> bool;
    auto load(QTextStream &in, const EncodingInfo &enc, Type type,
              const QUrl &url = QUrl()) -> bool;
    auto loadPLS(QTextStream &in, const QUrl &url = QUrl()) -> bool;
    auto loadM3U(QTextStream &in, const QUrl &url = QUrl()) -> bool;
    auto loadCue(QTextStream &in, const QUrl &url = QUrl()) -> bool;
};

Q_DECLARE_METATYPE(Playlist)

auto operator << (QDataStream &out, const Playlist &pl) -> QDataStream&;
auto operator >> (QDataStream &out, Playlist &pl) -> QDataStream&;

#endif // PLAYLIST_HPP
