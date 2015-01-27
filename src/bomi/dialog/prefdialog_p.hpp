#ifndef PREFDIALOG_P_HPP
#define PREFDIALOG_P_HPP

#include "enum/mousebehavior.hpp"
#include "misc/keymodifieractionmap.hpp"
#include "misc/simplelistmodel.hpp"
#include "misc/matchstring.hpp"
#include "player/pref.hpp"

class Menu;

static const int CategoryRole = Qt::UserRole + 1;
static const int WidgetRole = Qt::UserRole + CategoryRole;

struct ActionInfo { QString key, description; };

class PrefMenuTreeItem : public QTreeWidgetItem {
public:
    enum Column {
        Discription = 0, Shortcut1, Shortcut2, Shortcut3, Shortcut4, Id
    };
    auto isMenu() const -> bool { return m_action->menu() != 0; }
    auto isSeparator() const -> bool { return m_action->isSeparator(); }
    auto shortcut(int i) const -> QKeySequence {return m_shortcuts[i];}
    auto setShortcut(int idx, const QKeySequence &key) -> void;
    auto setShortcuts(const QList<QKeySequence> &keys) -> void;
    auto shortcuts() const -> QList<QKeySequence>;
    auto hasShortcut(const QKeySequence &key) -> bool;
    auto id() const -> QString { return m_id; }
    auto description() const -> QString { return m_desc; }
private:
    friend class PrefMenuTreeWidget;
    static auto create(Menu *menu, QVector<PrefMenuTreeItem*> &items,
                       const QString &prefix,
                       QVector<ActionInfo> &list) -> PrefMenuTreeItem*;
    PrefMenuTreeItem(Menu *menu, PrefMenuTreeItem *parent);
    PrefMenuTreeItem(QAction *action, PrefMenuTreeItem *parent);
    QAction *m_action; QString m_id, m_desc;
    std::array<QKeySequence, 4> m_shortcuts;
};

class PrefMenuTreeWidget : public QTreeWidget {
    Q_OBJECT
    Q_PROPERTY(Shortcuts value READ get WRITE set)
public:
    PrefMenuTreeWidget(QWidget *parent = nullptr);
    auto actionInfoList() const -> const QVector<ActionInfo>& { return m_actionInfos; }
    auto set(const Shortcuts &shortcuts) -> void
    {
        for (auto item : m_actionItems)
            item->setShortcuts(shortcuts[item->id()]);
    }
    auto get() -> Shortcuts
    {
        Shortcuts shortcuts;
        for (auto item : m_actionItems) {
            const auto keys = item->shortcuts();
            if (!keys.isEmpty())
                shortcuts[item->id()] = keys;
        }
        return shortcuts;
    }
    auto item(const QKeySequence &key) const -> PrefMenuTreeItem*
    {
        for (auto item : m_actionItems) {
            if (item->hasShortcut(key))
                return item;
        }
        return nullptr;
    }
private:
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
    Q_PROPERTY(MouseActionMap value READ get WRITE set)
public:
    PrefMouseActionTree(QWidget *parent = nullptr): QTreeWidget(parent) { }
    auto setActionList(const QVector<ActionInfo> *actions) -> void;
    auto set(const MouseActionMap &map) -> void;
    auto get() const -> MouseActionMap;
};

#endif // PREFDIALOG_P_HPP
