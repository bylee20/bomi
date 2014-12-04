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

class SubSearchPathModel : public SimpleListModel<MatchString> {
    Q_OBJECT
    Q_PROPERTY(QList<MatchString> value READ list WRITE setList)
public:
    SubSearchPathModel(QObject *parent = nullptr)
        : SimpleListModel(3, parent) { }
    auto edit(int row, int column, const QVariant &var) -> bool override;
    auto flags(int /*row*/, int /*column*/) const -> Qt::ItemFlags override
    { return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable; }
private:
    auto header(int column) const -> QString override;
    auto displayData(int r, int c) const -> QVariant override;
};


class SubSearchPathValidator : public QValidator {
    Q_OBJECT
public:
    SubSearchPathValidator(QObject *parent): QValidator(parent) { }
    auto validate(QString &input, int &/*pos*/) const -> State
        { return input.contains(u'/'_q) ? Invalid : Acceptable; }
};

class SubSearchPathDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    SubSearchPathDelegate(QObject *parent): QStyledItemDelegate(parent) { }
    auto createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                          const QModelIndex &index) const -> QWidget*
    {
        switch (index.column()) {
        case 0: {
            auto edit = new QLineEdit(parent);
            edit->setValidator(new SubSearchPathValidator(edit));
            return edit;
        } case 1: {
            auto cbox = new QComboBox(parent);
            cbox->addItems(QStringList() << tr("Text") << tr("RegEx"));
            return cbox;
        } case 2: {
            auto cbox = new QComboBox(parent);
            cbox->addItems(QStringList() << tr("Yes") << tr("No"));
            return cbox;
        } default:
            return nullptr;
        }
    }
    auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void
    {
        auto m = qobject_cast<const SubSearchPathModel*>(index.model());
        if (!editor || !m || !m->isValid(index))
            return;
        const auto &t = m->at(index.row());
        switch (index.column()) {
        case 0:
            static_cast<QLineEdit*>(editor)->setText(t.string());
            break;
        case 1:
            static_cast<QComboBox*>(editor)->setCurrentIndex(t.isRegEx());
            break;
        case 2:
            static_cast<QComboBox*>(editor)->setCurrentIndex(t.isCaseInsensitive());
            break;
        default: break;
        }
    }
    auto setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const -> void
    {
        auto m = qobject_cast<SubSearchPathModel*>(model);
        if (!editor || !m || !m->isValid(index))
            return;
        int r = index.row(), c = index.column();
        switch (index.column()) {
        case 0:
            m->edit(r, c, static_cast<QLineEdit*>(editor)->text());
            break;
        case 1: case 2:
            m->edit(r, c, !!static_cast<QComboBox*>(editor)->currentIndex());
            break;
        default:
            break;
        }
    }
    auto updateEditorGeometry(QWidget *w, const QStyleOptionViewItem &opt,
                              const QModelIndex &/*index*/) const -> void
    {
        if (w)
            w->setGeometry(opt.rect);
    }
};

#endif // PREFDIALOG_P_HPP
