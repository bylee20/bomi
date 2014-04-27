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
    typedef QHash<QString, Menu*> MenuHash;
    typedef QHash<QString, QAction*> ActionHash;
    typedef QHash<QString, ActionGroup*> GroupHash;

    inline Menu &operator() (const char *key) const
    { return operator ()(_L(key)); }
    inline Menu &operator() (const char *key, const QString &title) const
    { return operator ()(_L(key), title); }
    inline QAction *operator[] (const char *key) const
    {return operator[] (_L(key));}
    inline ActionGroup *g(const char *key) const { return g(_L(key)); }
    inline QAction *a(const char *key) const { return a(_L(key)); }
    inline Menu *m(const char *key) const { return m(_L(key)); }
    inline Menu *addMenu(const char *key) { return addMenu(_L(key)); }
    inline QAction *addActionToGroup(const char *key,
                                     bool ch = false, const char *g = "")
    { return addActionToGroup(_L(key), ch, _L(g)); }
    inline QAction *addActionToGroup(QAction *action, const char *key,
                                     const char *g = "")
    { return addActionToGroup(action, _L(key), _L(g)); }
    inline QAction *addAction(const char *key, bool ch = false)
    { return addAction(_L(key), ch); }
    inline ActionGroup *addGroup(const char *key) { return addGroup(_L(key)); }
    inline QAction *a(const char *key, const QString &text) const
    { return a(_L(key), text); }

    inline Menu &operator() (const QString &key) const {return *m(key);}
    inline Menu &operator() (const QString &key, const QString &title) const
    { auto &menu = *m(key); menu.setTitle(title); return menu; }
    inline QAction *operator[] (const QString &key) const {return a(key);}
    inline ActionGroup *g(const QString &key = _L("")) const {return m_g[key];}
    inline QAction *a(const QString &key) const {return m_a[key];}
    inline Menu *m(const QString &key) const {return m_m[key];}
    inline Menu *addMenu(const QString &key)
    {
        Menu *m = m_m[key] = new Menu(key, this);
        QMenu::addMenu(m); m_ids[key] = m->menuAction(); return m;
    }
    inline QAction *addActionToGroup(QAction *action, const QString &key,
                                     const QString &g = _L(""))
    { return m_ids[key] = addGroup(g)->addAction(addAction(action, key)); }
    inline QAction *addActionToGroup(const QString &key, bool ch = false,
                                     const QString &g = _L(""))
    { return m_ids[key] = addGroup(g)->addAction(addAction(key, ch)); }
    inline QAction *addAction(QAction *a, const QString &k)
    { QMenu::addAction(a); a->setParent(this); return m_a[k] = m_ids[k] = a; }
    inline QAction *addAction(const QString &key, bool ch = false)
    {
        QAction *a = m_a[key] = QMenu::addAction(key);
        a->setCheckable(ch); m_ids[key] = a; return a;
    }
    inline QAction *addActionToGroupWithoutKey(const QString &name,
                                               bool ch = false,
                                               const QString &g = _L(""))
    {
        QAction *a = QMenu::addAction(name);
        a->setCheckable(ch); return addGroup(g)->addAction(a);
    }
    inline ActionGroup *addGroup(const QString &key)
    {
        ActionGroup *g = m_g.value(key, 0);
        return g ? g : (m_g[key] = new ActionGroup(this));
    }
    inline QAction *a(const QString &key, const QString &text) const
    { auto action = a(key); action->setText(text); return action; }
    inline QString id(QAction *a) const {return m_a.key(a, QString());}
    inline QString id(Menu *menu) const {return m_m.key(menu, QString());}
    inline QString id() const {return m_id;}
    QMenu *copied(QWidget *parent = nullptr);
    QList<QMenu*> copies() const {return m_copies;}
    inline void setEnabledSync(bool enabled)
    { setEnabled(enabled); for (QMenu *m : m_copies) m->setEnabled(enabled); }
    inline void syncTitle()
    {
        for (QMenu *m : m_copies) m->setTitle(title());
        for (Menu *m : m_m) m->syncTitle();
    }
    inline void syncActions()
    {
        for (QMenu *m : m_copies)
            m->addActions(actions());
        emit actionsSynchronized();
    }
    inline QHash<QString, QAction*> ids() const {return m_ids;}
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

#endif // MENU_HPP
