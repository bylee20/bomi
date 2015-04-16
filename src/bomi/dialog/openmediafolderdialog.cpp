#include "openmediafolderdialog.hpp"
#include "ui_openmediafolderdialog.h"
#include "player/playlist.hpp"
#include "misc/objectstorage.hpp"
#include <QFileIconProvider>
#include <QCollator>
#include "tmp/algorithm.hpp"

enum ListRole {
    Type = Qt::UserRole + 1, Path
};

struct OpenMediaFolderDialog::Data {
    OpenMediaFolderDialog *p = nullptr;
    Ui::OpenMediaFolderDialog ui;
    bool generating = false;
    QFileIconProvider icons;
    ObjectStorage storage;

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


    static auto sort(QFileInfoList &list) -> QFileInfoList&
    {
        QCollator c;
        c.setNumericMode(true);
        tmp::sort(list, [&](auto &lhs, auto &rhs)
            { return c.compare(lhs.fileName(), rhs.fileName()) < 0; });
        return list;
    }

    auto open(const QDir &dir, const QString &root, bool recursive) -> void
    {
        static const auto filters = _ToNameFilter(MediaExt);
        auto files = dir.entryInfoList(filters, QDir::Files);
        sort(files);
        for (int i=0; i<files.size(); ++i) {
            const auto path = files[i].absoluteFilePath();
            auto item = new QListWidgetItem(path.mid(root.size() + 1), ui.list);
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
            item->setData(Path, path);
            item->setCheckState(box->isChecked() ? Qt::Checked : Qt::Unchecked);
        }
        if (recursive) {
            auto subs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            sort(subs);
            for (auto &sub : subs)
                open(QDir(sub.absoluteFilePath()), root, recursive);
        }
    }

    auto updateList() -> void
    {
        ui.list->clear();
        const auto folder = ui.folder->text();
        if (!folder.isEmpty()) {
            const QDir dir(folder);
            open(dir, dir.absolutePath(), ui.recursive->isChecked());
        }
        updateOpenButton();
    }
};

OpenMediaFolderDialog::OpenMediaFolderDialog(QWidget *parent, const QString &key)
    : QDialog(parent)
    , d(new Data)
{
    d->p = this;
    d->ui.setupUi(this);
    _SetWindowTitle(this, tr("Open Folder"));

    connect(d->ui.get, &PathButton::folderSelected, this, &OpenMediaFolderDialog::setFolder);
    connect(d->ui.videos, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.videos, checked); });
    connect(d->ui.audios, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.audios, checked); });
    connect(d->ui.images, &QAbstractButton::toggled,
            this, [=] (bool checked) { d->checkList(d->ui.images, checked); });
    connect(d->ui.list, &QListWidget::itemChanged,
            this, [=] () { d->updateOpenButton(); });
    connect(d->ui.recursive, &QCheckBox::toggled, this, [=] () { d->updateList(); });

    d->ui.dbb->button(QDialogButtonBox::Open)->setEnabled(false);
    adjustSize();

    d->ui.videos->setChecked(true);
    d->ui.images->setChecked(true);

    d->storage.setObject(this, "OpenMediaFolderDialog-"_a % key);
    d->storage.add(d->ui.videos);
    d->storage.add(d->ui.audios);
    d->storage.add(d->ui.images);
    d->storage.restore();
}

OpenMediaFolderDialog::~OpenMediaFolderDialog() {
    d->storage.save();
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
        setFolder(d->ui.get->getFolder());
    if (d->ui.folder->text().isEmpty())
        return Rejected;
    d->storage.save();
    return QDialog::exec();

}
