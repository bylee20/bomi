#ifndef MENU_HPP
#define MENU_HPP

#include "misc/actiongroup.hpp"
#include <QMenu>

class StepActionPair;                   class StepAction;

class Menu : public QMenu {
public:
    using Group = ActionGroup;
    using MenuHash = QMap<QString, Menu*>;
    using ActionHash = QMap<QString, QAction*>;
    using GroupHash = QMap<QString, Group*>;
    using StepActionPairHash = QMap<QString, StepActionPair*>;

    auto operator() (const QString &key) const -> Menu& { return *m(key); }
    auto operator[] (const QString &key) const -> QAction* { return a(key); }
    auto g() const -> Group* { return m_g[QString()]; }
    auto g(const QString &key) const -> Group* { return m_g[key]; }
    auto m(const QString &key) const -> Menu* { return m_m[key]; }
    auto a(const QString &key) const -> QAction* { return m_a[key]; }
    auto addMenu(const QString &k) -> Menu*
        { Menu *m = new Menu(k, this); QMenu::addMenu(m); return m_m[k] = m; }
    auto addActionToGroup(QAction *action, const QString &key,
                          const QString &group = u""_q) -> QAction*
        { return addGroup(group)->addAction(addAction(action, key)); }
    auto addActionToGroup(const QString &key, bool checkable = false,
                          const QString &group = u""_q) -> QAction*
        { return addGroup(group)->addAction(addAction(key, checkable)); }
    auto addAction(QAction *a, const QString &key) -> QAction*
        { QMenu::addAction(a); a->setParent(this); return m_a[key] = a; }
    auto addAction(const QString &k, bool ch = false) -> QAction*
        { QAction *a = m_a[k] = QMenu::addAction(k); a->setCheckable(ch); return a; }
    auto addActionNoKey(const QString &name, bool checkable = false,
                        const QString &group = u""_q) -> QAction*;
    auto addGroup(const QString &k) -> Group*
        { Group *g = m_g.value(k, 0); return g ? g : (m_g[k] = new Group(this)); }
    auto key(QAction *a) const -> QString { return m_a.key(a, QString()); }
    auto key(Menu *menu) const -> QString { return m_m.key(menu, QString()); }
    auto key() const -> QString {return m_key;}
    auto s(const QString &key = QString()) const -> StepActionPair* { return m_s[key]; }
    auto addStepActionPair(const QString &inc, const QString &dec,
                           const QString &pair,
                           const QString &group = QString()) -> StepActionPair*;
    auto addStepActionPair(const QString &pair,
                           const QString &group = QString()) -> StepActionPair*
        { return addStepActionPair(u"increase"_q, u"decrease"_q, pair, group); }
    auto beginGroup() const { return m_g.cbegin(); }
    auto endGroup() const { return m_g.cend(); }
    auto action(const QString &key) const -> QAction* { return m_a.value(key); }
protected:
    Menu(const QString &key, QWidget *parent);
private:
    GroupHash m_g;
    ActionHash m_a;
    MenuHash m_m;
    StepActionPairHash m_s;
    const QString m_key;
};

inline auto Menu::addActionNoKey(const QString &text, bool checkable,
                                 const QString &group) -> QAction*
{
    QAction *action = QMenu::addAction(text);
    action->setCheckable(checkable);
    return addGroup(group)->addAction(action);
}

#endif // MENU_HPP
