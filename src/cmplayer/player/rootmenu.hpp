#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "widget/menu.hpp"

using Shortcuts = QHash<QString, QList<QKeySequence>>;

class Pref;

class RootMenu : public Menu {
    Q_OBJECT
private:
    struct ArgAction { QString argument; QAction *action = nullptr; };
public:
    enum Preset {Current, CMPlayer, Movist};
    RootMenu();
    RootMenu(const RootMenu &) = delete;
    ~RootMenu() {obj = nullptr;}
    auto retranslate() -> void;
    auto longId(QAction *action) const -> QString {return m_ids.value(action);}
    auto action(const QString &longId) const -> QAction*
        { return m_actions.value(longId).action; }
    auto action(const QKeySequence &shortcut) const -> QAction*
        { return m_keymap.value(shortcut); }
    auto resetKeyMap() -> void { m_keymap.clear(); fillKeyMap(this); }
    auto shortcuts() const -> Shortcuts;
    auto setShortcuts(const Shortcuts &shortcuts) -> void;
    static auto instance() -> RootMenu& {return *obj;}
    static auto execute(const QString &longId,
                        const QString &argument = QString()) -> bool;
private:
    auto fillId(Menu *menu, const QString &parent) -> void;
    auto fillKeyMap(Menu *menu) -> void;
    static RootMenu *obj;
    QHash<QAction*, QString> m_ids;
    QHash<QString, ArgAction> m_actions;
    QMap<QKeySequence, QAction*> m_keymap;
};

#endif // ROOTMENU_HPP
