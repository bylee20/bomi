#include "prefdialog_p.hpp"
#include "player/rootmenu.hpp"
#include "tmp/algorithm.hpp"
#include "enum/mousebehavior.hpp"
#include <QHeaderView>
#include <QElapsedTimer>
#include <QScrollBar>

PrefMenuTreeItem::PrefMenuTreeItem(QAction *action, QVector<ActionInfo> &list, PrefMenuTreeItem *parent)
    : QTreeWidgetItem(parent, action->menu() ? Menu : action->isSeparator() ? Separator : Action)
{
    m_id = action->objectName();
    if (action->menu())
        m_name = action->menu()->title();
    else if (!action->isSeparator())
        m_name = action->text();
    if (parent && !parent->description().isEmpty())
        m_desc = parent->description() % " > "_a % m_name;
    else
        m_desc = m_name;

    switch (type()) {
    case Menu:
        for (auto a : action->menu()->actions()) {
            if (!a->objectName().isEmpty())
                new PrefMenuTreeItem(a, list, this);
        }
        break;
    case Action:
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

auto PrefMenuTreeItem::setShortcut(const Shortcut &s) -> bool
{
    return _Change(m_shortcut, s);
}

/******************************************************************************/

PrefMenuTreeWidget::PrefMenuTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    RootMenu &root = RootMenu::instance();
    auto item = new PrefMenuTreeItem(root.menuAction(), m_actionInfos, nullptr);
    addTopLevelItems(item->takeChildren());
    delete item;
    m_actionInfos.prepend({QString(), tr("Unused")});
    header()->resizeSection(0, 200);
    connect(this, &QTreeWidget::itemChanged, this, &PrefMenuTreeWidget::changed);
}

auto PrefMenuTreeWidget::set(const ShortcutMap &map) -> void {
    bool changed = false;
    const int h = horizontalScrollBar()->value();
    const int v = verticalScrollBar()->value();
    QList<QTreeWidgetItem*> expanded;
    for_recursive([&] (auto i) {
        changed |= i->setShortcut(map.shortcut(i->id()));
        if (i->isExpanded())
            expanded.push_back(i);
    });
    if (changed) {
        reset();
        for (auto i : expanded)
            i->setExpanded(true);
        horizontalScrollBar()->setValue(h);
        verticalScrollBar()->setValue(v);
        emit this->changed();
    }
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
            combo->addItem(actions->at(i).desc);
        return combo;
    }
    auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void
    {
        if (!editor)
            return;
        const int idx = index.model()->data(index, Qt::EditRole).toInt();
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
        model->setData(index, idx, Qt::EditRole);
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
    struct ChildItem : public QTreeWidgetItem {
        KeyModifier modifier;
        const QVector<ActionInfo> *actions = nullptr;
        int index = 0;
        auto data(int column, int role) const -> QVariant final
        {
            switch (column) {
            case 0:
                if (role != Qt::DisplayRole)
                    return QVariant();
                if (modifier == KeyModifier::None)
                    return qApp->translate("PrefDialog", "No modifier");
                return Key((int)modifier).toString(Key::NativeText);
            case 1:
                if (role == Qt::DisplayRole)
                    return actions ? actions->at(index).desc : QVariant();
                if (role == Qt::EditRole)
                    return index;
                return QVariant();
            default:
                return QVariant();
            }
        }
        auto setData(int column, int role, const QVariant &value) -> void final
        {
            if (column != 1 || role != Qt::EditRole || !actions)
                return;
            if (_Change(index, qBound(0, value.toInt(), actions->size() - 1)))
                emitDataChanged();
        }
    };
public:
    MouseActionItem(MouseBehavior mb, const QVector<ActionInfo> *actions,
                    QTreeWidget *parent)
        : QTreeWidgetItem(parent), m_actions(actions), m_mb(mb)
    {
        setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        setText(0, MouseBehaviorInfo::description(mb));
        for (auto mod : mods) {
            auto sub = new ChildItem;
            sub->modifier = mod;
            sub->actions = m_actions;
            sub->setFlags(flags() | Qt::ItemIsEditable);
            addChild(sub);
        }
    }
    auto behavior() const -> MouseBehavior { return m_mb; }
    auto compare(const KeyModifierActionMap &map) const -> bool
    {
        for (int i = 0; i < mods.size(); ++i) {
            const auto idx = static_cast<const ChildItem*>(child(i))->index;
            if (map[mods[i]] != m_actions->at(idx).id)
                return false;
        }
        return true;
    }
    auto set(const KeyModifierActionMap &map) -> bool
    {
        bool changed = false;
        for (int i = 0; i < mods.size(); ++i) {
            const auto id = map[mods[i]];
            int idx = 0;
            for (int j = 0; j < m_actions->size(); ++j) {
                if (m_actions->at(j).id == id) {
                    idx = j;
                    break;
                }
            }
            auto c = static_cast<ChildItem*>(child(i));
            changed |= _Change(c->index, idx);
        }
        return changed;
    }
    auto get() const -> KeyModifierActionMap
    {
        KeyModifierActionMap map;
        for (int i = 0; i < mods.size(); ++i) {
            const auto idx = child(i)->data(1, Qt::EditRole).toInt();
            map[mods[i]] = m_actions->at(idx).id;
        }
        return map;
    }
private:
    const QVector<ActionInfo> *m_actions = nullptr;
    MouseBehavior m_mb;
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
    for (auto one : all) {
        if (one.value == MouseBehavior::NoBehavior)
            continue;
        const auto item = new MouseActionItem(one.value, acts, this);
        switch (item->behavior()) {
        case MouseBehavior::Extra1Click:
            item->setText(1, qApp->translate("PrefDialog",
                "Typically denoted as 'Back' button"));
            break;
        case MouseBehavior::Extra2Click:
            item->setText(1, qApp->translate("PrefDialog",
                "Typically denoted as 'Forward' button"));
            break;
        case MouseBehavior::LeftClick:
            item->setText(1, qApp->translate("PrefDialog",
                "Triggered with delay of %1sec to be distingushed from double click")
                .arg(qApp->doubleClickInterval()*1e-3, 0, 'g', 3));
            break;
        default:
            break;
        }
    }
    setItemDelegate(new MouseActionDelegate(acts, this));
    expandAll();
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

auto PrefMouseActionTree::compare(const QVariant &var) const -> bool
{
    const auto map = var.value<MouseActionMap>();
    for (int i = 0; i < topLevelItemCount(); ++i) {
        const auto item = static_cast<MouseActionItem*>(topLevelItem(i));
        if (!item->compare(map[item->behavior()]))
            return false;
    }
    return true;
}

auto PrefMouseActionTree::set(const MouseActionMap &map) -> void
{
    bool changed = false;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        const auto item = static_cast<MouseActionItem*>(topLevelItem(i));
        changed |= item->set(map[item->behavior()]);
    }
    if (!changed)
        return;
    reset();
    expandAll();
    emit this->changed();
}

auto PrefMouseActionTree::get() const -> MouseActionMap
{
    MouseActionMap map;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        const auto item = static_cast<MouseActionItem*>(topLevelItem(i));
        map[item->behavior()] = item->get();
    }
    return map;
}

/******************************************************************************/

enum PrefStepColumn {
    StepText = 0, StepValue = 1
};

enum PrefStepRole {
    StepInfoRole = Qt::UserRole
};

Q_DECLARE_METATYPE(const StepInfo*);

class PrefStepItem : public QTreeWidgetItem {
public:
    PrefStepItem(const StepInfo *info, const QString &desc, QTreeWidgetItem *parent)
        : QTreeWidgetItem(parent, UserType + 1)
    {
        m_info = info;
        setText(StepText, desc);
        setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
    }
    auto data(int column, int role) const -> QVariant
    {
        if (column != StepValue)
            return QTreeWidgetItem::data(column, role);
        switch (role) {
        case Qt::DisplayRole:
            return m_info->text(m_value, ChangeValue::Increase, false);
        case Qt::EditRole:
            return m_value;
        case StepInfoRole:
            return QVariant::fromValue(m_info);
        default:
            return QVariant();
        }
    }
    auto setValue(double value) -> void
    {
        if (_Change(m_value, value))
            emitDataChanged();
    }
    auto setData(int column, int role, const QVariant &value) -> void final
    {
        if (column != StepValue || role != Qt::EditRole)
            QTreeWidgetItem::setData(column, role, value);
        setValue(value.toDouble());
    }
    auto value() const -> double { return m_value; }
    auto info() const -> const StepInfo* { return m_info; }
private:
    const StepInfo *m_info = nullptr;
    double m_value = 0;
};


class PrefStepDelegate : public QStyledItemDelegate {
public:
    PrefStepDelegate(PrefStepTreeWidget *parent)
        : QStyledItemDelegate(parent) {

    }
    auto createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                      const QModelIndex &index) const -> QWidget*
    {
        if (!index.parent().isValid() || index.column() != 1)
            return nullptr;
        const auto info = index.data(StepInfoRole).value<const StepInfo*>();
        Q_ASSERT(info);
        QAbstractSpinBox *spin = nullptr;
        if (info->precision) {
            auto editor = new QDoubleSpinBox(parent);
            editor->setRange(0, info->max);
            editor->setSingleStep(info->single);
            editor->setSuffix(info->suffix());
            editor->setDecimals(info->precision);
            editor->setValue(index.data(Qt::EditRole).toDouble());
            spin = editor;
        } else {
            auto editor = new QSpinBox(parent);
            editor->setRange(0, qRound(info->max));
            editor->setSingleStep(qRound(info->single));
            editor->setSuffix(info->suffix());
            editor->setValue(qRound(index.data(Qt::EditRole).toDouble()));
            spin = editor;
        }
        spin->setAccelerated(true);
        return spin;
    }
    auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void
    {
        if (!editor)
            return;
        const auto data = index.data(Qt::EditRole);
        if (auto e = qobject_cast<QDoubleSpinBox*>(editor))
            e->setValue(data.toDouble());
        else
            static_cast<QSpinBox*>(editor)->setValue(qRound(data.toDouble()));
    }
    auto setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const -> void
    {
        if (!editor)
            return;
        if (auto e = qobject_cast<QDoubleSpinBox*>(editor))
            model->setData(index, e->value(), Qt::EditRole);
        else
            model->setData(index, double(static_cast<QSpinBox*>(editor)->value()), Qt::EditRole);
    }
    auto updateEditorGeometry(QWidget *w, const QStyleOptionViewItem &opt,
                              const QModelIndex &/*index*/) const -> void
        { if (w) w->setGeometry(opt.rect); }
};

PrefStepTreeWidget::PrefStepTreeWidget(QWidget *p)
    : QTreeWidget(p)
{
    clear();
    setColumnCount(2);

    Steps steps;

#define add(name, desc) { \
    m_items.push_back(new PrefStepItem(steps.name.info(), desc, parent));}

    auto parent = new QTreeWidgetItem(this);
    parent->setText(0, tr("Playback"));
    add(seek1_sec, tr("Seek Step 1"));
    add(seek2_sec, tr("Seek Step 2"));
    add(seek3_sec, tr("Seek Step 3"));
    add(speed_pct, tr("Speed"));

    parent = new QTreeWidgetItem(this);
    parent->setText(0, tr("Video"));
    add(aspect_ratio, tr("Aspect Ratio"));
    add(zoom_pct, tr("Zoom"));
    add(video_offset_pct, tr("Move Position"));
    add(color_pct, tr("Color Adjustment"));

    parent = new QTreeWidgetItem(this);
    parent->setText(0, tr("Audio"));
    add(volume_pct, tr("Volume"));
    add(amp_pct, tr("Amp"));
    add(audio_sync_sec, tr("Sync Delay"));

    parent = new QTreeWidgetItem(this);
    parent->setText(0, tr("Subtitle"));
    add(sub_pos_pct, tr("Position"));
    add(sub_sync_sec, tr("Sync Delay"));

    expandAll();
    resizeColumnToContents(0);
    setItemDelegate(new PrefStepDelegate(this));
    connect(this, &QTreeWidget::itemChanged, this, &PrefStepTreeWidget::changed);
}

auto PrefStepTreeWidget::get() const -> Steps
{
    Steps steps;
    for (auto item : m_items)
        item->info()->value(&steps).set(item->value());
    return steps;
}

auto PrefStepTreeWidget::set(const Steps &steps) -> void
{
    for (auto item : m_items)
        item->setValue(item->info()->value(&steps).get());
}
