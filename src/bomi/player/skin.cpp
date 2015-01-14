#include "skin.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(Skin)

Skin::Data::Data() {
    auto append = [] (const QString &dir, QStringList &dirs)
    {
        if (!dirs.contains(dir)) {
            _Debug("Add directory to search skin in: %%", dir);
            dirs.push_back(dir);
        }
    };

#ifdef BOMI_SKINS_PATH
    append(QString::fromLocal8Bit(BOMI_SKINS_PATH), dirs);
#endif
    append(QDir::homePath() % "/.bomi/skins"_a, dirs);
    append(qApp->applicationDirPath() % "/skins"_a, dirs);
    auto path = qgetenv("BOMI_SKINS_PATH");
    if (!path.isEmpty())
        append(QString::fromLocal8Bit(path.data(), path.size()), dirs);

#ifdef BOMI_IMPORTS_PATH
    append(QString::fromLocal8Bit(BOMI_IMPORTS_PATH), qmls);
#endif
    append(QDir::homePath() % "/.bomi/imports"_a, qmls);
    append(qApp->applicationDirPath() % "/imports"_a, qmls);
    path = qgetenv("BOMI_IMPORTS_PATH");
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
        if (dir.exists()) {
            const auto names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (auto &name : names) {
                const QFileInfo source(dir.filePath(name % "/bomi.qml"_a));
                if (source.exists())
                    d->skins[name] = source;
            }
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
