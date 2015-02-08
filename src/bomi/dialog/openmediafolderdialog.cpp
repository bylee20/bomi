#include "openmediafolderdialog.hpp"
#include "ui_openmediafolderdialog.h"
#include "player/playlist.hpp"

enum ListRole {
    Type = Qt::UserRole + 1, Path
};

struct OpenMediaFolderDialog::Data {
    OpenMediaFolderDialog *p = nullptr;
    Ui::OpenMediaFolderDialog ui;
    bool generating = false;
    QFileIconProvider icons;
    QString key;

    auto setCheckedTypes(const QString &types) -> void
    {
        ui.videos->setChecked(types.contains('v'_q));
        ui.images->setChecked(types.contains('i'_q));
        ui.audios->setChecked(types.contains('a'_q));
    }

    auto checkedTypes() const -> QString
    {
        QString types;
        if (ui.videos->isChecked())
            types.append('v'_q);
        if (ui.audios->isChecked())
            types.append('a'_q);
        if (ui.images->isChecked())
            types.append('i'_q);
        return types;
    }

    auto updateOpenButton() -> void
    {
        if (!generating) {
            ui.dbb->button(QDialogButtonBox::Open)->setEnabled(false);
            for (int i=0; i<ui.list->count(); ++i) {
                if (ui.list->item(i)->checkState() == Qt::Checked) {
                    ui.dbb->button(QDialogButtonBox::Open)->setEnabled(true);
                    break;
                }
            }
        }
    }

    auto checkList(QCheckBox *box, bool checked) -> void
    {
        for (int i=0; i<ui.list->count(); ++i) {
            auto item = ui.list->item(i);
            if (item->data(Type).value<QCheckBox*>() == box)
                item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    }

    auto updateList() -> void
    {
        const auto folder = ui.folder->text();
        if (!folder.isEmpty()) {
            ui.list->clear();
            QDir dir(folder);
            static const auto filters = _ToNameFilter(MediaExt);
            const auto files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
            for (int i=0; i<files.size(); ++i) {
                auto item = new QListWidgetItem(files[i].fileName(), ui.list);
                QCheckBox *box = nullptr;
                item->setCheckState(Qt::Unchecked);
                const QString suffix = files[i].suffix();
                if (_IsSuffixOf(VideoExt, suffix))
                    box = ui.videos;
                else if (_IsSuffixOf(AudioExt, suffix))
                    box = ui.audios;
                else if (_IsSuffixOf(ImageExt, suffix))
                    box = ui.images;
                Q_ASSERT(box);
                item->setIcon(icons.icon(files[i]));
                item->setData(Type, QVariant::fromValue(box));
                item->setData(Path, files[i].absoluteFilePath());
                item->setCheckState(box->isChecked() ? Qt::Checked : Qt::Unchecked);
            }
            updateOpenButton();
        }
    }
    auto getFolder() -> void
    {
        p->setFolder(_GetOpenDir(p, tr("Open Folder")));
    }
};

#define GROUP "OpenMediaFolderDialog_"_a

OpenMediaFolderDialog::OpenMediaFolderDialog(QWidget *parent, const QString &key)
    : QDialog(parent)
    , d(new Data)
{
    d->p = this;
    d->ui.setupUi(this);
    _SetWindowTitle(this, tr("Open Folder"));

    connect(d->ui.get, &QAbstractButton::clicked,
            this, [=] () { d->getFolder(); });
    connect(d->ui.videos, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.videos, checked); });
    connect(d->ui.audios, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.audios, checked); });
    connect(d->ui.images, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.images, checked); });
    connect(d->ui.list, &QListWidget::itemChanged,
            this, [=] () { d->updateOpenButton(); });
    d->ui.dbb->button(QDialogButtonBox::Open)->setEnabled(false);
    adjustSize();

    d->key = key;
    QSettings settings;
    settings.beginGroup(GROUP % d->key);
    d->setCheckedTypes(settings.value(u"checked_types"_q, u"vi"_q).toString());
    settings.endGroup();

}

OpenMediaFolderDialog::~OpenMediaFolderDialog() {
    delete d;
}

auto OpenMediaFolderDialog::playlist() const -> Playlist
{
    Playlist list;
    for (int i=0; i<d->ui.list->count(); ++i) {
        const auto item = d->ui.list->item(i);
        if (item->checkState() == Qt::Checked)
            list.append(item->data(Path).toString());
    }
    return list;
}

auto OpenMediaFolderDialog::setFolder(const QString &folder) -> void
{
    if (!folder.isEmpty() && d->ui.folder->text() != folder) {
        d->ui.folder->setText(folder);
        d->updateList();
    }
}

auto OpenMediaFolderDialog::exec() -> int
{
    if (d->ui.folder->text().isEmpty())
        d->getFolder();
    if (d->ui.folder->text().isEmpty())
        return Rejected;
    QSettings settings;
    settings.beginGroup(GROUP % d->key);
    settings.setValue(u"checked_types"_q, d->checkedTypes());
    settings.endGroup();
    return QDialog::exec();

}
