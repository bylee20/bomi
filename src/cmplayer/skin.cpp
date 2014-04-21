#include "skin.hpp"

Skin::Data::Data() {
    auto append = [] (const QString &dir, QStringList &dirs) { if (!dirs.contains(dir)) dirs << dir; };

#ifdef CMPLAYER_SKINS_PATH
    append(QString::fromLocal8Bit(CMPLAYER_SKINS_PATH), dirs);
#endif
    append(QDir::homePath() % _L("/.cmplayer/skins"), dirs);
    append(QCoreApplication::applicationDirPath().toLocal8Bit() % _L("/skins"), dirs);
    auto path = qgetenv("CMPLAYER_SKINS_PATH");
    if (!path.isEmpty())
        append(QString::fromLocal8Bit(path.data(), path.size()), dirs);

#ifdef CMPLAYER_IMPORTS_PATH
    append(QString::fromLocal8Bit(CMPLAYER_IMPORTS_PATH), qmls);
#endif
    append(QDir::homePath() % _L("/.cmplayer/imports"), qmls);
    append(QCoreApplication::applicationDirPath().toLocal8Bit() % _L("/imports"), qmls);
    path = qgetenv("CMPLAYER_IMPORTS_PATH");
    if (!path.isEmpty())
        append(QString::fromLocal8Bit(path.data(), path.size()), qmls);
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

void Skin::apply(QQuickView *view, const QString &name) {
    if (data()->skins.isEmpty())
        names(true);
    auto imports = view->engine()->importPathList();
    for (auto path : data()->qmls) {
        if (!imports.contains(path))
            view->engine()->addImportPath(path);
    }
    const auto skin = Skin::source(name);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setSource(QUrl::fromLocalFile(skin.absoluteFilePath()));
}
