#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "widget/menu.hpp"

class ShortcutMap;

class RootMenu : public Menu {
    Q_DECLARE_TR_FUNCTIONS(RootMenu)
public:
    enum Preset {Current, bomi, Movist};
    ~RootMenu();
    auto retranslate() -> void;
    auto description(const QString &id) const -> QString;
    auto action(const QString &id) const -> QAction*;
    auto action(const QKeySequence &key) const -> QAction*;
    auto setShortcutMap(const ShortcutMap &map) -> void;
    auto resolve(const QString &id) const -> QString;
    static auto instance() -> RootMenu&;
    static auto finalize() -> void;
    static auto execute(const QString &id) -> bool;
    static auto dumpInfo() -> void;
private:
    RootMenu();
    struct Data;
    Data *d;
};

#endif // ROOTMENU_HPP
