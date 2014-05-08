#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "menu.hpp"

typedef QHash<QString, QList<QKeySequence> > Shortcuts;

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
    auto update(const Pref &p) -> void;
    static inline RootMenu &instance() {return *obj;}
    auto longId(QAction *action) const -> QString {return m_ids.value(action);}
    QAction *action(const QString &longId) const {return m_actions.value(longId).action;}
    QAction *action(const QKeySequence &shortcut) const {return m_keymap.value(shortcut);}
    inline auto resetKeyMap() -> void {m_keymap.clear(); fillKeyMap(this);}
    auto shortcuts() const -> Shortcuts;
    auto setShortcuts(const Shortcuts &shortcuts) -> void;
    static bool execute(const QString &longId, const QString &argument = QString());
private:
    template<class N>
    inline static void setActionAttr(QAction *act, const QVariant &data
            , const QString &text, N textValue, bool sign = true) {
        act->setData(data);
        act->setText(text.arg(toString(textValue, sign)));
    }

    inline static void setActionStep(QAction *plus, QAction *minus
            , const QString &text, int value, double textRate = -1.0) {
        if (textRate < 0) {
            plus->setText(text.arg(toString(value, true)));
            minus->setText(text.arg(toString(-value, true)));
        } else {
            plus->setText(text.arg(toString(value*textRate, true)));
            minus->setText(text.arg(toString(-value*textRate, true)));
        }
        plus->setData(value);
        minus->setData(-value);
    }

    inline static void setVideoPropStep(Menu &menu, const QString &key
            , VideoColor::Type prop, const QString &text, int step) {
        setActionAttr(menu[key + "+"], QVariantList() << prop << step, text, step);
        setActionAttr(menu[key + "-"], QVariantList() << prop << -step, text, -step);
    }
    auto fillId(Menu *menu, const QString &parent) -> void;
    auto fillKeyMap(Menu *menu) -> void;
    static RootMenu *obj;
    QHash<QAction*, QString> m_ids;
    QHash<QString, ArgAction> m_actions;
    QMap<QKeySequence, QAction*> m_keymap;
};

#endif // ROOTMENU_HPP
