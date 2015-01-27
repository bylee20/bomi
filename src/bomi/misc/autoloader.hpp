#ifndef AUTOLOADER_HPP
#define AUTOLOADER_HPP

#include "matchstring.hpp"
#include "enum/autoloadmode.hpp"
#include "player/mrl.hpp"

struct Autoloader {
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto autoload(const Mrl &mrl, ExtType type) const -> QStringList;

    QList<MatchString> search_paths;
    bool enabled = false;
    AutoloadMode mode = AutoloadMode::Matched;
private:
    auto tryDir(const Mrl &mrl, ExtType type, const QDir &dir) const -> QStringList;
};

Q_DECLARE_METATYPE(Autoloader)

class AutoloaderWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(Autoloader value READ value WRITE setValue)
public:
    AutoloaderWidget(QWidget *parent = nullptr);
    ~AutoloaderWidget();
    auto value() const -> Autoloader;
    auto setValue(const Autoloader &value) -> void;
private:
    struct Data;
    Data *d;
};

#endif // AUTOLOADER_HPP
