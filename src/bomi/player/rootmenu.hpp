#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "widget/menu.hpp"

using Shortcuts = QMap<QString, QList<QKeySequence>>;

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
    auto resetKeyMap() -> void { m_keymap.clear(); fillKeyMap(this); }
    auto shortcuts() const -> Shortcuts;
    auto setShortcuts(const Shortcuts &shortcuts) -> void;
    static auto instance() -> RootMenu& {return *obj;}
    static auto execute(const QString &longId,
                        const QString &argument = QString()) -> bool;
private:
    auto fillKeyMap(Menu *menu) -> void;
    static RootMenu *obj;
    QMap<QKeySequence, QAction*> m_keymap;
    struct Data;
    Data *d;
};

#endif // ROOTMENU_HPP
