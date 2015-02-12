#include "prefdialog_p.hpp"
#include "player/rootmenu.hpp"
#include "tmp/algorithm.hpp"
#include "enum/mousebehavior.hpp"
#include <QHeaderView>

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

auto PrefMenuTreeItem::create(Menu *menu, QVector<PrefMenuTreeItem*> &items,
                              const QString &prefix, QVector<ActionInfo> &list)
-> PrefMenuTreeItem*
{
    RootMenu &root = RootMenu::instance();
    const auto actions = menu->actions();
    QList<QTreeWidgetItem*> children;
    for (int i=0; i<actions.size(); ++i) {
        const auto action = actions[i];
        const auto id = root.id(action);
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
                list.append({ id, child->m_desc });
            }
        }
    }
    if (children.isEmpty())
        return 0;
    PrefMenuTreeItem *item = new PrefMenuTreeItem(menu, 0);
    item->addChildren(children);
    return item;
}

/******************************************************************************/

PrefMenuTreeWidget::PrefMenuTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    RootMenu &root = RootMenu::instance();
    auto item = PrefMenuTreeItem::create(&root, m_actionItems, QString(), m_actionInfos);
    addTopLevelItems(item->takeChildren());
    delete item;
    m_actionInfos.prepend({QString(), tr("Unused")});
    header()->resizeSection(0, 200);
    connect(this, &QTreeWidget::itemChanged, this, &PrefMenuTreeWidget::changed);
}

/******************************************************************************/

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

/******************************************************************************/

static constexpr auto ModRole = Qt::UserRole + 1;
static constexpr auto ActionIndexRole = Qt::UserRole + 2;

class MouseActionDelegate : public QStyledItemDelegate {
    const QVector<ActionInfo> *actions = nullptr;
public:
    MouseActionDelegate(const QVector<ActionInfo> *actions,
                        PrefMouseActionTree *parent)
        : QStyledItemDelegate(parent), actions(actions) { }
    auto createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                          const QModelIndex &index) const -> QWidget*
    {
        if (!index.parent().isValid() || index.column() != 1)
            return nullptr;
        auto combo = new QComboBox(parent);
        for (int i = 0; i < actions->size(); ++i)
            combo->addItem(actions->at(i).description);
        return combo;
    }
    auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void
    {
        if (!editor)
            return;
        const int idx = index.model()->data(index, ActionIndexRole).toInt();
        auto combox = static_cast<QComboBox*>(editor);
        combox->setCurrentIndex(idx);
    }
    auto setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const -> void
    {
        if (!editor)
            return;
        auto combo = static_cast<QComboBox*>(editor);
        const auto idx = combo->currentIndex();
        model->setData(index, idx, ActionIndexRole);
        model->setData(index, actions->at(idx).description);
    }
    auto updateEditorGeometry(QWidget *w, const QStyleOptionViewItem &opt,
                              const QModelIndex &/*index*/) const -> void
    {
        if (w)
            w->setGeometry(opt.rect);
    }
};

static const auto mods = QList<KeyModifier>() << KeyModifier::None
                                              << KeyModifier::Ctrl
                                              << KeyModifier::Shift
                                              << KeyModifier::Alt;

class MouseActionItem : public QTreeWidgetItem {

public:
    MouseActionItem(MouseBehavior mb, const QVector<ActionInfo> *actions,
                    QTreeWidget *parent)
        : QTreeWidgetItem(parent), m_actions(actions)
    {
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        setText(0, MouseBehaviorInfo::description(mb));
        for (auto mod : mods) {
            auto sub = new QTreeWidgetItem;
            sub->setData(0, ModRole, QVariant::fromValue(mod));
            if (mod != KeyModifier::None) {
                const QKeySequence key((int)mod);
                sub->setText(0, key.toString(QKeySequence::NativeText));
            } else
                sub->setText(0, qApp->translate("PrefDialog", "No modifier"));
            sub->setFlags(flags() | Qt::ItemIsEditable);
            addChild(sub);
        }
    }
    auto compare(const KeyModifierActionMap &map) const -> bool
    {
        for (int i = 0; i < mods.size(); ++i) {
            const auto idx = child(i)->data(1, ActionIndexRole).toInt();
            if (map[mods[i]] != m_actions->at(idx).key)
                return false;
        }
        return true;
    }
    auto set(const KeyModifierActionMap &map) -> void
    {
        for (int i = 0; i < mods.size(); ++i) {
            const auto id = map[mods[i]];
            int idx = 0;
            for (int j = 0; j < m_actions->size(); ++j) {
                if (m_actions->at(j).key == id) {
                    idx = j;
                    break;
                }
            }
            auto sub = child(i);
            sub->setText(1, m_actions->at(idx).description);
            sub->setData(1, ActionIndexRole, idx);
        }
    }
    auto get() const -> KeyModifierActionMap
    {
        KeyModifierActionMap map;
        for (int i = 0; i < mods.size(); ++i) {
            const auto idx = child(i)->data(1, ActionIndexRole).toInt();
            map[mods[i]] = m_actions->at(idx).key;
        }
        return map;
    }
private:
    const QVector<ActionInfo> *m_actions = nullptr;
};

PrefMouseActionTree::PrefMouseActionTree(QWidget *parent)
    : QTreeWidget(parent)
{
    connect(this, &QTreeWidget::itemChanged, this, &PrefMouseActionTree::changed);
}

auto PrefMouseActionTree::setActionList(const QVector<ActionInfo> *acts) -> void
{
    clear();
    setColumnCount(2);
    setHeaderLabels(QStringList() << tr("Behavior") << tr("Menu"));
    const auto all = MouseBehaviorInfo::items();
    for (auto one : all)
        addTopLevelItem(new MouseActionItem(one.value, acts, this));
    setItemDelegate(new MouseActionDelegate(acts, this));
    expandAll();
    resizeColumnToContents(0);
    auto setExp = [this] (MouseBehavior mb, const QString &exp)
        { topLevelItem((int)mb)->setText(1, exp); };
    setExp(MouseBehavior::Extra1Click,
           qApp->translate("PrefDialog", "Typically denoted as 'Back' button"));
    setExp(MouseBehavior::Extra2Click,
           qApp->translate("PrefDialog", "Typically denoted as 'Forward' button"));
}

auto PrefMouseActionTree::compare(const QVariant &var) const -> bool
{
    const auto map = var.value<MouseActionMap>();
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (!static_cast<MouseActionItem*>(topLevelItem(i))->compare(map[(MouseBehavior)i]))
            return false;
    }
    return true;
}

auto PrefMouseActionTree::set(const MouseActionMap &map) -> void
{
    for (int i = 0; i < topLevelItemCount(); ++i)
        static_cast<MouseActionItem*>(topLevelItem(i))->set(map[(MouseBehavior)i]);
}

auto PrefMouseActionTree::get() const -> MouseActionMap
{
    MouseActionMap map;
    for (int i = 0; i < topLevelItemCount(); ++i)
        map[(MouseBehavior)i] = static_cast<MouseActionItem*>(topLevelItem(i))->get();
    return map;
}
