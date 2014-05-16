#ifndef MENU_HPP
#define MENU_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "actiongroup.hpp"
#include "enums.hpp"

class Record;

class Menu : public QMenu {
    Q_OBJECT
public:
    static constexpr int IdRole = Qt::UserRole + 1;
    using MenuHash = QHash<QString, Menu*>;
    using ActionHash = QHash<QString, QAction*>;
    using GroupHash = QHash<QString, ActionGroup*>;

    auto operator() (const char *key) const -> Menu &;
    auto operator() (const char *key, const QString &title) const -> Menu&;
    auto operator[] (const char *key) const -> QAction*;
    auto g(const char *key) const -> ActionGroup* { return g(_L(key)); }
    auto a(const char *key) const -> QAction* { return a(_L(key)); }
    auto m(const char *key) const -> Menu* { return m(_L(key)); }
    auto addMenu(const char *key) -> Menu* { return addMenu(_L(key)); }
    auto addActionToGroup(const char *key, bool checkable = false,
                          const char *group = "") -> QAction*;
    auto addActionToGroup(QAction *action, const char *key,
                          const char *group = "") -> QAction*;
    auto addAction(const char *key, bool checkable = false) -> QAction*;
    auto addGroup(const char *key) -> ActionGroup* { return addGroup(_L(key)); }
    auto a(const char *key, const QString &text) const -> QAction*;

    auto operator() (const QString &key) const -> Menu& { return *m(key); }
    auto operator() (const QString &key, const QString &title) const -> Menu&;
    auto operator[] (const QString &key) const -> QAction* { return a(key); }
    auto g(const QString &key = _L("")) const -> ActionGroup*;
    auto a(const QString &key) const -> QAction* { return m_a[key]; }
    auto m(const QString &key) const -> Menu* { return m_m[key]; }
    auto addMenu(const QString &key) -> Menu*;
    auto addActionToGroup(QAction *action, const QString &key,
                          const QString &group = _L("")) -> QAction*;
    auto addActionToGroup(const QString &key, bool checkable = false,
                          const QString &group = _L("")) -> QAction*;
    auto addAction(QAction *action, const QString &key) -> QAction*;
    auto addAction(const QString &key, bool checkable = false) -> QAction*;
    auto addActionToGroupWithoutKey(const QString &name, bool checkable = false,
                                    const QString &group = _L("")) -> QAction*;
    auto addGroup(const QString &key) -> ActionGroup*;
    auto a(const QString &key, const QString &text) const -> QAction*;
    auto id(QAction *a) const -> QString { return m_a.key(a, QString()); }
    auto id(Menu *menu) const -> QString { return m_m.key(menu, QString()); }
    auto id() const -> QString {return m_id;}
    auto copied(QWidget *parent = nullptr) -> QMenu*;
    auto copies() const -> QList<QMenu*> { return m_copies; }
    auto setEnabledSync(bool enabled) -> void;
    auto syncTitle() -> void;
    auto syncActions() -> void;
    auto ids() const -> QHash<QString, QAction*> { return m_ids; }
signals:
    void actionsSynchronized();
protected:
    Menu(const QString &id, QWidget *parent);
private:
    GroupHash m_g;
    ActionHash m_a;
    MenuHash m_m;
    QHash<QString, QAction*> m_ids;
    QList<QMenu*> m_copies;
    const QString m_id;
    friend class MenuBar;
};

inline auto Menu::operator() (const char *key) const -> Menu &
{ return operator ()(_L(key)); }

inline auto Menu::operator() (const char *key,
                              const QString &title) const -> Menu&
{ return operator ()(_L(key), title); }

inline auto Menu::operator[] (const char *key) const -> QAction*
{return operator[] (_L(key));}

inline auto Menu::addActionToGroup(const char *key, bool checkable,
                                   const char *group) -> QAction*
{ return addActionToGroup(_L(key), checkable, _L(group)); }

inline auto Menu::addActionToGroup(QAction *action, const char *key,
                                   const char *group) -> QAction*
{ return addActionToGroup(action, _L(key), _L(group)); }

inline auto Menu::addAction(const char *key, bool checkable) -> QAction*
{ return addAction(_L(key), checkable); }

inline auto Menu::a(const char *key, const QString &text) const -> QAction*
{ return a(_L(key), text); }

inline auto Menu::operator() (const QString &key,
                              const QString &title) const -> Menu&
{ auto &menu = *m(key); menu.setTitle(title); return menu; }

inline auto Menu::g(const QString &key) const -> ActionGroup*
{ return m_g[key]; }

inline auto Menu::addMenu(const QString &key) -> Menu*
{
    Menu *menu = m_m[key] = new Menu(key, this);
    QMenu::addMenu(menu);
    m_ids[key] = menu->menuAction();
    return menu;
}

inline auto Menu::addActionToGroup(QAction *action, const QString &key,
                                 const QString &group) -> QAction*
{ return m_ids[key] = addGroup(group)->addAction(addAction(action, key)); }

inline auto Menu::addActionToGroup(const QString &key, bool checkable,
                                 const QString &group) -> QAction*
{ return m_ids[key] = addGroup(group)->addAction(addAction(key, checkable)); }

inline auto Menu::addAction(QAction *action, const QString &key) -> QAction*
{
    QMenu::addAction(action);
    action->setParent(this);
    return m_a[key] = m_ids[key] = action;
}

inline auto Menu::addAction(const QString &key, bool checkable) -> QAction*
{
    QAction *action = m_a[key] = QMenu::addAction(key);
    action->setCheckable(checkable);
    return m_ids[key] = action;
}

inline auto Menu::addActionToGroupWithoutKey(const QString &name,
                                             bool checkable,
                                             const QString &group) -> QAction*
{
    QAction *action = QMenu::addAction(name);
    action->setCheckable(checkable);
    return addGroup(group)->addAction(action);
}

inline auto Menu::addGroup(const QString &key) -> ActionGroup*
{
    ActionGroup *group = m_g.value(key, 0);
    return group ? group : (m_g[key] = new ActionGroup(this));
}

inline auto Menu::a(const QString &key, const QString &text) const -> QAction*
{ auto action = a(key); action->setText(text); return action; }

inline auto Menu::setEnabledSync(bool enabled) -> void
{
    setEnabled(enabled);
    for (auto menu : m_copies)
        menu->setEnabled(enabled);
}

inline auto Menu::syncTitle() -> void
{
    for (auto menu : m_copies)
        menu->setTitle(title());
    for (auto menu : m_m)
        menu->syncTitle();
}

inline auto Menu::syncActions() -> void
{
    for (auto menu : m_copies)
        menu->addActions(actions());
    emit actionsSynchronized();
}

#endif // MENU_HPP
