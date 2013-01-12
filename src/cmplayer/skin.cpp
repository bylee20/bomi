#include "skin.hpp"

Skin::Data::Data() {
#ifdef CMPLAYER_SKINS_PATH
	dirs << QString::fromLocal8Bit(CMPLAYER_SKINS_PATH);
#endif
	dirs << QDir::homePath() % _L("/.cmplayer/skins");
	dirs << QCoreApplication::applicationDirPath().toLocal8Bit() % _L("/skins");
	const QByteArray path = qgetenv("CMPLAYER_SKINS_PATH");
	if (!path.isEmpty())
		dirs << QString::fromLocal8Bit(path.data(), path.size());
}

QStringList Skin::names(bool reload/* = false*/) {
	auto d = data();
	if (d->skins.isEmpty() || reload) {
		d->skins.clear();
		for (auto &dirName : d->dirs) {
			const QDir dir(dirName);
			if (dir.exists()) {
				auto names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
				for (auto &name : names) {
					const QFileInfo source(dir.filePath(name + "/cmplayer.qml"));
					if (source.exists())
						d->skins[name] = source;
				}
			}
		}
	}
	return d->skins.keys();
}
