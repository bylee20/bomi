#include "skin.hpp"

Skin::Data::Data() {
    auto append = [] (const QString &dir, QStringList &dirs)
        { if (!dirs.contains(dir)) dirs << dir; };

#ifdef CMPLAYER_SKINS_PATH
    append(QString::fromLocal8Bit(CMPLAYER_SKINS_PATH), dirs);
#endif
    append(QDir::homePath() % "/.cmplayer/skins"_a, dirs);
    append(qApp->applicationDirPath().toLocal8Bit() % "/skins"_a, dirs);
    auto path = qgetenv("CMPLAYER_SKINS_PATH");
    if (!path.isEmpty())
        append(QString::fromLocal8Bit(path.data(), path.size()), dirs);

#ifdef CMPLAYER_IMPORTS_PATH
    append(QString::fromLocal8Bit(CMPLAYER_IMPORTS_PATH), qmls);
#endif
    append(QDir::homePath() % "/.cmplayer/imports"_a, qmls);
    append(qApp->applicationDirPath().toLocal8Bit() % "/imports"_a, qmls);
    path = qgetenv("CMPLAYER_IMPORTS_PATH");
    if (!path.isEmpty())
        append(QString::fromLocal8Bit(path.data(), path.size()), qmls);
}

auto Skin::names(bool reload/* = false*/) -> QStringList
{
    auto d = data();
    if (!d->skins.isEmpty() && !reload)
        return d->skins.keys();
    d->skins.clear();
    for (auto &dirName : d->dirs) {
        const QDir dir(dirName);
        if (!dir.exists())
            continue;
        const auto names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (auto &name : names) {
            const QFileInfo source(dir.filePath(name + "/cmplayer.qml"));
            if (source.exists())
                d->skins[name] = source;
        }
    }
    return d->skins.keys();
}

auto Skin::apply(QQuickView *view, const QString &name) -> void
{
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

auto Skin::source(const QString &name) -> QFileInfo
{
    auto it = data()->skins.find(name);
    if (it != data()->skins.end())
        return it.value();
    return QFileInfo();
}
