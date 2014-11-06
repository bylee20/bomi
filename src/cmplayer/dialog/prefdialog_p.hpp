#ifndef PREFDIALOG_P_HPP
#define PREFDIALOG_P_HPP

#include "enum/mousebehavior.hpp"
#include "misc/keymodifieractionmap.hpp"

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
    static auto makeRoot(QTreeWidget *parent)
        -> T<QVector<PrefMenuTreeItem*>, QVector<ActionInfo>>;
private:
    static auto create(Menu *menu, QVector<PrefMenuTreeItem*> &items,
                       const QString &prefix,
                       QVector<ActionInfo> &list) -> PrefMenuTreeItem*;
    PrefMenuTreeItem(Menu *menu, PrefMenuTreeItem *parent);
    PrefMenuTreeItem(QAction *action, PrefMenuTreeItem *parent);
    QAction *m_action; QString m_id, m_desc;
    std::array<QKeySequence, 4> m_shortcuts;
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
public:
    PrefMouseActionTree(QWidget *parent = nullptr): QTreeWidget(parent) { }
    auto setActionList(const QVector<ActionInfo> *actions) -> void;
    auto set(const MouseActionMap &map) -> void;
    auto get() const -> MouseActionMap;
};

#endif // PREFDIALOG_P_HPP
