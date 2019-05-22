#ifndef FILENAMEGENERATOR_HPP
#define FILENAMEGENERATOR_HPP

#include "player/mrl.hpp"

struct FileNameGenerator {
    static auto toolTip() -> QString;
    auto get(const QString &folder, const QString &format,
             const QString &suffix) const -> QString;
    QDateTime dateTime;
    QTime start, end;
    QString mediaName;
    Mrl mrl;
    quint64 unix_ = 0;
};

#endif // FILENAMEGENERATOR_HPP
