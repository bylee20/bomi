#ifndef PREFDIALOG_P_HPP
#define PREFDIALOG_P_HPP

#include "player/shortcutmap.hpp"
#include "enum/mousebehavior.hpp"
#include "misc/keymodifieractionmap.hpp"
#include "misc/simplelistmodel.hpp"
#include "misc/matchstring.hpp"
#include "player/pref.hpp"
#include <QAction>
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

class Menu;

static const int CategoryRole = Qt::UserRole + 1;
static const int WidgetRole = Qt::UserRole + CategoryRole;

struct ActionInfo { QString key, description; };

using Key = QKeySequence;

class PrefMenuTreeItem : public QTreeWidgetItem {
public:
    enum Type {
        NoType = QTreeWidgetItem::Type,
        Menu = UserType, Separator, Action
    };
    enum Column {
        Name = 0, Shortcut1, Shortcut2, Shortcut3, Shortcut4, Id
    };
    auto isMenu() const -> bool { return type() == Menu; }
    auto isSeparator() const -> bool { return type() == Separator; }
    auto key(int i) const -> Key {return m_shortcut.key(i);}
    auto setKey(int idx, const QKeySequence &key) -> void;
    auto setShortcut(const Shortcut &s) -> void;
    auto shortcut() const -> Shortcut { return m_shortcut; }
    auto contains(const Key &key) -> bool { return m_shortcut.contains(key); }
    auto id() const -> QString { return m_id; }
    auto description() const -> QString { return m_desc; }
    auto reset() -> void;
private:
    auto data(int column, int role) const -> QVariant final;
    friend class PrefMenuTreeWidget;
    PrefMenuTreeItem(QAction *action, QVector<PrefMenuTreeItem*> &items,
                     QVector<ActionInfo> &list, PrefMenuTreeItem *parent);
    QString m_desc, m_name, m_id; Shortcut m_shortcut;
};

class PrefMenuTreeWidget : public QTreeWidget {
    Q_OBJECT
    Q_PROPERTY(ShortcutMap value READ get WRITE set NOTIFY changed)
public:
    PrefMenuTreeWidget(QWidget *parent = nullptr);
    auto actionInfoList() const -> const QVector<ActionInfo>& { return m_actionInfos; }
    auto set(const ShortcutMap &map) -> void
    {
        for (auto item : m_actionItems)
            item->setShortcut(map.shortcut(item->id()));
    }
    auto get() -> ShortcutMap
    {
        ShortcutMap map;
        for (auto item : m_actionItems)
            map.insert(item->shortcut());
        return map;
    }
    auto item(const QKeySequence &key) const -> PrefMenuTreeItem*
    {
        for (auto item : m_actionItems) {
            if (item->contains(key))
                return item;
        }
        return nullptr;
    }
    auto compare(const QVariant &var) const -> bool
    {
        const auto map = var.value<ShortcutMap>();
        for (auto item : m_actionItems) {
            if (item->shortcut() != map.shortcut(item->id()))
                return false;
        }
        return true;
    }
signals:
    void changed();
private:
//    auto recursive(QTreeWidgetItem *item)
    QVector<PrefMenuTreeItem*> m_actionItems;
    QVector<ActionInfo> m_actionInfos;
};

// from clementine's preferences dialog

class PrefDelegate : public QStyledItemDelegate {
public:
    PrefDelegate(QObject* parent): QStyledItemDelegate(parent) { }
    auto sizeHint(const QStyleOptionViewItem& option,
                  const QModelIndex& index) const -> QSize;
    auto paint(QPainter *p, const QStyleOptionViewItem &o,
               const QModelIndex &idx) const -> void;
private:
    static constexpr int kBarThickness = 2;
    static constexpr int kBarMarginTop = 3;
    static auto drawHeader(QPainter *painter, const QRect &rect,
                           const QFont &font, const QPalette &palette,
                           const QString &text) -> void;
};

class PrefMouseActionTree : public QTreeWidget {
    Q_OBJECT
    Q_PROPERTY(MouseActionMap value READ get WRITE set NOTIFY changed)
public:
    PrefMouseActionTree(QWidget *parent = nullptr);
    auto setActionList(const QVector<ActionInfo> *actions) -> void;
    auto set(const MouseActionMap &map) -> void;
    auto get() const -> MouseActionMap;
    auto compare(const QVariant &var) const -> bool;
signals:
    void changed();
};


#endif // PREFDIALOG_P_HPP
