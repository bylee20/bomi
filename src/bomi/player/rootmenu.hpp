#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "widget/menu.hpp"

class ShortcutMap;

class RootMenu : public Menu {
    Q_OBJECT
public:
    enum Preset {Current, bomi, Movist};
    ~RootMenu();
    auto retranslate() -> void;
    auto description(const QString &longId) const -> QString;
    auto action(const QString &longId) const -> QAction*;
    auto action(const QKeySequence &shortcut) const -> QAction*
        { return m_keymap.value(shortcut); }
    auto argument(QAction *action) const -> QString;
    auto setShortcutMap(const ShortcutMap &map) -> void;
    static auto instance() -> RootMenu&;
    static auto finalize() -> void;
    static auto execute(const QString &id) -> bool;
    static auto dumpInfo() -> void;
private:
    RootMenu();
    QMap<QKeySequence, QAction*> m_keymap;
    struct Data;
    Data *d;
};

#endif // ROOTMENU_HPP
