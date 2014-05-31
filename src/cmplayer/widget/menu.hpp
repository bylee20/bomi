#ifndef MENU_HPP
#define MENU_HPP

#include "misc/actiongroup.hpp"

class StepActionPair;                   class StepAction;

class Menu : public QMenu {
    Q_OBJECT
public:
    static constexpr int IdRole = Qt::UserRole + 1;
    using MenuHash = QHash<QString, Menu*>;
    using ActionHash = QHash<QString, QAction*>;
    using GroupHash = QHash<QString, ActionGroup*>;
    using StepActionPairHash = QHash<QString, StepActionPair*>;

    auto operator() (const QString &key) const -> Menu& { return *m(key); }
    auto operator() (const QString &key, const QString &title) const -> Menu&;
    auto operator[] (const QString &key) const -> QAction* { return a(key); }
    auto g(const QString &key = QString()) const -> ActionGroup*;
    auto a(const QString &key) const -> QAction* { return m_a[key]; }
    auto m(const QString &key) const -> Menu* { return m_m[key]; }
    auto addMenu(const QString &key) -> Menu*;
    auto addActionToGroup(QAction *action, const QString &key,
                          const QString &group = u""_q) -> QAction*;
    auto addActionToGroup(const QString &key, bool checkable = false,
                          const QString &group = u""_q) -> QAction*;
    auto addAction(QAction *action, const QString &key) -> QAction*;
    auto addAction(const QString &key, bool checkable = false) -> QAction*;
    auto addActionToGroupWithoutKey(const QString &name, bool checkable = false,
                                    const QString &group = u""_q) -> QAction*;
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
    auto s(const QString &key = QString()) const -> StepActionPair*;
    auto addStepActionPair(const QString &inc, const QString &dec,
                           const QString &pair,
                           const QString &group = QString()) -> StepActionPair*;
    auto addStepActionPair(const QString &pair,
                           const QString &group = QString()) -> StepActionPair*;
signals:
    void actionsSynchronized();
protected:
    Menu(const QString &id, QWidget *parent);
private:
    GroupHash m_g;
    ActionHash m_a;
    MenuHash m_m;
    StepActionPairHash m_s;
    QHash<QString, QAction*> m_ids;
    QList<QMenu*> m_copies;
    const QString m_id;
    friend class MenuBar;
};

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

inline auto Menu::s(const QString &key) const -> StepActionPair*
{
    return m_s[key];
}

inline auto Menu::addStepActionPair(const QString &pair,
                                    const QString &group) -> StepActionPair*
{
    return addStepActionPair(u"increase"_q, u"decrease"_q, pair, group);
}

#endif // MENU_HPP
