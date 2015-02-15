#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "widget/menu.hpp"

class ShortcutMap;

class RootMenu : public Menu {
    Q_OBJECT
public:
    enum Preset {Current, bomi, Movist};
    RootMenu();
    RootMenu(const RootMenu &) = delete;
    ~RootMenu();
    auto retranslate() -> void;
    auto id(QAction *action) const -> QString;
    auto description(const QString &longId) const -> QString;
    auto action(const QString &longId) const -> QAction*;
    auto action(const QKeySequence &shortcut) const -> QAction*
        { return m_keymap.value(shortcut); }
    auto setShortcutMap(const ShortcutMap &map) -> void;
    static auto instance() -> RootMenu& {return *obj;}
    static auto execute(const QString &longId,
                        const QString &argument = QString()) -> bool;
private:
    static RootMenu *obj;
    QMap<QKeySequence, QAction*> m_keymap;
    struct Data;
    Data *d;
};

#endif // ROOTMENU_HPP
