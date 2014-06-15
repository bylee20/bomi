#include "prefdialog_p.hpp"
#include "player/rootmenu.hpp"
#include "tmp/algorithm.hpp"

PrefMenuTreeItem::PrefMenuTreeItem(Menu *menu, PrefMenuTreeItem *parent)
    : QTreeWidgetItem(parent)
    , m_action(menu->menuAction())
{
    setText(Discription, menu->title());
}

PrefMenuTreeItem::PrefMenuTreeItem(QAction *action, PrefMenuTreeItem *parent)
    : QTreeWidgetItem(parent)
    , m_action(action)
{
    Q_ASSERT(action->menu() == 0);
    setText(Discription, m_action->text());
}

auto PrefMenuTreeItem::setShortcut(int idx, const QKeySequence &key) -> void
{
    m_shortcuts[idx] = key;
    setText(idx + Shortcut1, key.toString(QKeySequence::NativeText));
}

auto PrefMenuTreeItem::setShortcuts(const QList<QKeySequence> &keys) -> void
{
    for (int i = 0; i<(int)m_shortcuts.size(); ++i) {
        m_shortcuts[i] = (i < keys.size()) ? keys[i] : QKeySequence();
        setText(i+1, m_shortcuts[i].toString(QKeySequence::NativeText));
    }
}

auto PrefMenuTreeItem::hasShortcut(const QKeySequence &key) -> bool
{
    return !key.isEmpty() && tmp::contains(m_shortcuts, key);
}

auto PrefMenuTreeItem::shortcuts() const -> QList<QKeySequence>
{
    QList<QKeySequence> shortcuts;
    for (auto &key : m_shortcuts) {
        if (!key.isEmpty())
            shortcuts.push_back(key);
    }
    return shortcuts;
}

auto PrefMenuTreeItem::makeRoot(QTreeWidget *parent)
-> T<QVector<PrefMenuTreeItem*>, QVector<MouseAction>>
{
    RootMenu &root = RootMenu::instance();
    QVector<PrefMenuTreeItem*> items;
    QVector<MouseAction> info;
    auto item = create(&root, items, QString(), info);
    parent->addTopLevelItems(item->takeChildren());
    delete item;
    return _T(items, info);
}

auto PrefMenuTreeItem::create(Menu *menu, QVector<PrefMenuTreeItem*> &items,
                              const QString &prefix, QVector<MouseAction> &list)
-> PrefMenuTreeItem*
{
    RootMenu &root = RootMenu::instance();
    const auto actions = menu->actions();
    QList<QTreeWidgetItem*> children;
    for (int i=0; i<actions.size(); ++i) {
        const auto action = actions[i];
        const auto id = root.longId(action);
        if (!id.isEmpty()) {
            if (action->menu()) {
                auto menu = qobject_cast<Menu*>(action->menu());
                Q_ASSERT(menu);
                const QString subprefix = prefix % menu->title()  % u" > "_q;
                if (auto child = create(menu, items, subprefix, list))
                    children.push_back(child);
            } else {
                auto child = new PrefMenuTreeItem(action, 0);
                child->m_desc = prefix % action->text();
                child->setText(Id, child->m_id = id);
                items.push_back(child);
                children.push_back(child);
                list.append({ child->m_desc, id });
            }
        }
    }
    if (children.isEmpty())
        return 0;
    PrefMenuTreeItem *item = new PrefMenuTreeItem(menu, 0);
    item->addChildren(children);
    return item;
}

auto PrefDelegate::sizeHint(const QStyleOptionViewItem& option,
                            const QModelIndex& index) const -> QSize
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if (index.data(CategoryRole).toBool())
        size.rheight() *= 2;
    return size;
}

auto PrefDelegate::paint(QPainter *p, const QStyleOptionViewItem &o,
                         const QModelIndex &idx) const -> void
{
    if (idx.data(CategoryRole).toBool())
        drawHeader(p, o.rect, o.font, o.palette, idx.data().toString());
    else
        QStyledItemDelegate::paint(p, o, idx);
}

auto PrefDelegate::drawHeader(QPainter *painter, const QRect &rect,
                              const QFont &font, const QPalette &palette,
                              const QString &text) -> void
{
    painter->save();

    // Bold font
    QFont bold_font(font);
    bold_font.setBold(true);
    QFontMetrics metrics(bold_font);

    QRect text_rect(rect);
    text_rect.setHeight(metrics.height());
    const auto by = (rect.height() - text_rect.height()
                     - kBarThickness - kBarMarginTop) / 2;
    text_rect.moveTop(rect.top() + by);
    text_rect.setLeft(text_rect.left() + 3);

    // Draw text
    painter->setFont(bold_font);
    painter->drawText(text_rect, text);

    // Draw a line underneath
    const QPoint start(rect.left(), text_rect.bottom() + kBarMarginTop);
    const QPoint end(rect.right(), start.y());

    painter->setRenderHint(QPainter::Antialiasing, true);
    const auto color = palette.color(QPalette::Disabled, QPalette::Text);
    painter->setPen(QPen(color, kBarThickness, Qt::SolidLine, Qt::RoundCap));
    painter->setOpacity(0.5);
    painter->drawLine(start, end);

    painter->restore();
}
