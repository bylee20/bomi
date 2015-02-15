#include "prefdialog_p.hpp"
#include "player/rootmenu.hpp"
#include "tmp/algorithm.hpp"
#include "enum/mousebehavior.hpp"
#include <QHeaderView>

PrefMenuTreeItem::PrefMenuTreeItem(QAction *action, QVector<PrefMenuTreeItem*> &items, QVector<ActionInfo> &list, PrefMenuTreeItem *parent)
    : QTreeWidgetItem(parent, action->menu() ? Menu : action->isSeparator() ? Separator : Action)
{
    RootMenu &root = RootMenu::instance();
    m_id = root.id(action);
    if (action->menu())
        m_name = action->menu()->title();
    else if (!action->isSeparator())
        m_name = action->text();
    if (parent)
        m_desc = parent->description() % " > "_a % m_name;
    else
        m_desc = m_name;

    switch (type()) {
    case Menu:
        for (auto a : action->menu()->actions()) {
            if (root.id(a).isEmpty())
                continue;
            new PrefMenuTreeItem(a, items, list, this);
        }
        break;
    case Action:
        items.push_back(this);
        list.append({ m_id, m_desc });
        break;
    case Separator:
        break;
    }
}

auto PrefMenuTreeItem::setKey(int idx, const QKeySequence &key) -> void
{
    auto keys = m_shortcut.keys();
    while (idx >= keys.size())
        keys.push_back(Key());
    keys[idx] = key;
    m_shortcut.setKeys(keys);
    emitDataChanged();
}

auto PrefMenuTreeItem::data(int column, int role) const -> QVariant
{
    if (role == Qt::FontRole) {
        QFont font;
        if (!m_shortcut.isDefault())
            font.setItalic(!font.italic());
        return font;
    }
    if (role != Qt::DisplayRole)
        return QVariant();
    switch (column) {
    case Name:
        return m_name;
    case Shortcut1:
    case Shortcut2:
    case Shortcut3:
    case Shortcut4:
        return m_shortcut.key(column - Shortcut1);
    case Id:
        return m_shortcut.id();
    default:
        return QVariant();
    }
}

auto PrefMenuTreeItem::reset() -> void
{
    if (m_shortcut.isDefault())
        return;
    m_shortcut.reset();
    emitDataChanged();
}

auto PrefMenuTreeItem::setShortcut(const Shortcut &s) -> void
{
    if (_Change(m_shortcut, s))
        emitDataChanged();
}

/******************************************************************************/

PrefMenuTreeWidget::PrefMenuTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    RootMenu &root = RootMenu::instance();
    auto item = new PrefMenuTreeItem(root.menuAction(), m_actionItems, m_actionInfos, nullptr);
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
