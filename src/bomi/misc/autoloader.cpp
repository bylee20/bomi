#include "autoloader.hpp"
#include "json.hpp"
#include "ui_autoloaderwidget.h"
#include "simplelistmodel.hpp"
#include <QStyledItemDelegate>

#define JSON_CLASS Autoloader
static const auto jio = JIO(JE(enabled), JE(mode), JE(search_paths));
JSON_DECLARE_FROM_TO_FUNCTIONS

auto Autoloader::autoload(const Mrl &mrl, ExtType type) const -> QStringList
{
    if (!mrl.isLocalFile() || !enabled)
        return QStringList();
    const QFileInfo fileInfo(mrl.toLocalFile());
    auto root = fileInfo.dir();
    auto loaded = tryDir(fileInfo, type, root);
    auto list = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto &path : search_paths) {
        for (auto &one : list) {
            if (path.match(one)) {
                auto dir = root;
                if (dir.cd(one))
                    loaded += tryDir(fileInfo, type, dir);
            }
        }
    }
    return loaded;
}

auto Autoloader::tryDir(const QFileInfo &fileInfo, ExtType type,
                        const QDir &dir) const -> QStringList
{
    Q_ASSERT(enabled);
    if (!dir.exists())
        return QStringList();
    QStringList files;
    const auto filter = _ToNameFilter(type);
    const auto all = dir.entryInfoList(filter, QDir::Files, QDir::Name);
    const auto base = fileInfo.completeBaseName();
    for (int i = 0; i < all.size(); ++i) {
        if (all[i].fileName() == fileInfo.fileName())
            continue;
        if (mode != AutoloadMode::Folder) {
            if (mode == AutoloadMode::Matched) {
                if (base != all[i].completeBaseName())
                    continue;
            } else if (!all[i].fileName().contains(base))
                continue;
        }
        files.push_back(all[i].absoluteFilePath());
    }
    return files;
}

/******************************************************************************/

class SearchPathModel : public SimpleListModel<MatchString> {
    Q_DECLARE_TR_FUNCTIONS(SearchPathModel)
public:
    SearchPathModel(QObject *parent = nullptr)
        : SimpleListModel(3, parent) { }
    auto edit(int row, int column, const QVariant &var) -> bool override
    {
        auto &t = get(row);
        switch (column) {
        case 0: {
            auto str = var.toString();
            if (t.string() != str) {
                t.setString(str);
                return true;
            }
            return false;
        } case 1: {
            auto rx = var.toBool();
            if (t.isRegEx() != rx) {
                t.setRegEx(rx);
                return true;
            }
            return false;
        } case 2: {
            auto cis = var.toBool();
            if (t.isCaseInsensitive() != cis) {
                t.setCaseInsensitive(cis);
                return true;
            }
            return false;
        } default:
            return false;
        }
    }
    auto flags(int /*row*/, int /*column*/) const -> Qt::ItemFlags override
    { return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable; }
private:
    auto header(int column) const -> QString override
    {
        switch (column) {
        case 0:
            return tr("Directory name");
        case 1:
            return tr("Type");
        case 2:
            return tr("Case sensitive");
        default:
            return QString();
        }
    }
    auto displayData(int r, int c) const -> QVariant override
    {
        switch (c) {
        case 0:
            return at(r).string();
        case 1:
            return at(r).isRegEx() ? tr("RegEx") : tr("Text");
        case 2:
            return at(r).isCaseSensitive() ? tr("Yes") : tr("No");
        default:
            return QString();
        }
    }
};

class SearchPathValidator : public QValidator {
public:
    SearchPathValidator(QObject *parent): QValidator(parent) { }
    auto validate(QString &input, int &/*pos*/) const -> State
        { return input.contains(u'/'_q) ? Invalid : Acceptable; }
};

class SearchPathDelegate : public QStyledItemDelegate {
    Q_DECLARE_TR_FUNCTIONS(Autoloader)
public:
    SearchPathDelegate(QObject *parent): QStyledItemDelegate(parent) { }
    auto createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                          const QModelIndex &index) const -> QWidget*
    {
        switch (index.column()) {
        case 0: {
            auto edit = new QLineEdit(parent);
            edit->setValidator(new SearchPathValidator(edit));
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
        auto m = static_cast<const SearchPathModel*>(index.model());
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
        auto m = static_cast<SearchPathModel*>(model);
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

struct AutoloaderWidget::Data {
    Ui::AutoloaderWidget ui;
    SearchPathModel *searchPaths = nullptr;
};

AutoloaderWidget::AutoloaderWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    d->ui.setupUi(this);
    d->searchPaths = new SearchPathModel(this);

    d->ui.search_paths->setModel(d->searchPaths);
    d->ui.search_paths->setItemDelegate(new SearchPathDelegate(this));
    d->ui.search_path_browse->setEditor(d->ui.search_path_edit);
    d->ui.search_path_edit->setValidator(new SearchPathValidator(this));
    connect(d->ui.search_path_edit, &QLineEdit::textChanged,
            [this] (const QString &text) {
        d->ui.search_path_add->setEnabled(!text.isEmpty());
    });
    connect(d->ui.search_path_add, &QPushButton::clicked, [this] () {
        const auto text = d->ui.search_path_edit->text();
        if (text.isEmpty() || text.contains(u'/'_q))
            return;
        MatchString str(text);
        str.setCaseSensitive(d->ui.search_case_sensitive->isChecked());
        str.setRegEx(d->ui.search_regex->isChecked());
        d->searchPaths->append(str);
    });
    connect(d->ui.search_path_remove, &QPushButton::clicked, [this] () {
        const int idx = d->ui.search_paths->currentIndex().row();
        d->searchPaths->remove(idx);
    });

    auto signal = &AutoloaderWidget::valueChanged;
    PLUG_CHANGED(d->ui.enabled);
    PLUG_CHANGED(d->ui.mode);
    PLUG_CHANGED(d->searchPaths);
}

AutoloaderWidget::~AutoloaderWidget()
{
    delete d;
}

auto AutoloaderWidget::setValue(const Autoloader &value) -> void
{
    d->searchPaths->setList(value.search_paths);
    d->ui.enabled->setChecked(value.enabled);
    d->ui.mode->setCurrentEnum(value.mode);
}

auto AutoloaderWidget::value() const -> Autoloader
{
    Autoloader al;
    al.enabled = d->ui.enabled->isChecked();
    al.mode = d->ui.mode->currentEnum();
    al.search_paths = d->searchPaths->list();
    return al;
}
