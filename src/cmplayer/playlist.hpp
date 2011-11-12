#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include "mrl.hpp"
#include <QtCore/QSettings>

class QFile;		class QDir;

class Playlist : public QList<Mrl> {
public:
	enum Type {Unknown, PLS, M3U};
	Playlist();
	Playlist(const Playlist &rhs);
	Playlist(const Mrl &mrl);
	Playlist(const QList<Mrl> &rhs);
	void save(const QString &prefix, QSettings *set) const;
	void load(const QString &prefix, QSettings *set);
	bool save(const QString &filePath, Type type = Unknown) const;
	bool load(const QString &filePath, const QString &enc = QString(), Type type = Unknown);
	bool load(const Mrl &url, const QString &enc = QString(), Type type = Unknown);
	bool load(QFile *file, const QString &enc = QString(), Type type = Unknown);
	Playlist &loadAll(const QDir &dir);
private:
	static Type getType(const QString &fileName);
	bool savePLS(QFile *file) const;
	bool saveM3U(QFile *file) const;
	bool loadPLS(QFile *file, const QString &enc);
	bool loadM3U(QFile *file, const QString &enc);
};

#endif // PLAYLIST_HPP
