#ifndef PREFDIALOG_P_HPP
#define PREFDIALOG_P_HPP

class Menu;

static const int CategoryRole = Qt::UserRole + 1;
static const int WidgetRole = Qt::UserRole + CategoryRole;

class PrefMenuTreeItem : public QTreeWidgetItem {
    using MouseAction = QPair<QString, QString>;
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
    auto id() const -> QString { return m_id; }
    static auto makeRoot(QTreeWidget *parent,
                         QList<MouseAction> &info) -> QList<PrefMenuTreeItem*>;
private:
    static auto create(Menu *menu, QList<PrefMenuTreeItem*> &items,
                       const QString &prefix,
                       QList<MouseAction> &list) -> PrefMenuTreeItem*;
    PrefMenuTreeItem(Menu *menu, PrefMenuTreeItem *parent);
    PrefMenuTreeItem(QAction *action, PrefMenuTreeItem *parent);
    QAction *m_action; QString m_id;
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

#endif // PREFDIALOG_P_HPP
