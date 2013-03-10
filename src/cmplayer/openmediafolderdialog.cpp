#include "openmediafolderdialog.hpp"
#include "ui_openmediafolderdialog.h"
#include "playlist.hpp"
#include "appstate.hpp"
#include "info.hpp"

enum ListRole {
	Type = Qt::UserRole + 1, Path
};

struct OpenMediaFolderDialog::Data {
	Ui::OpenMediaFolderDialog ui;
	bool generating = false;
	QFileIconProvider icons;
};

OpenMediaFolderDialog::OpenMediaFolderDialog(QWidget *parent) :
QDialog(parent), d(new Data) {
	d->ui.setupUi(this);
	const auto types = AppState::get().open_folder_types;
	d->ui.videos->setChecked(types.contains('v'));
	d->ui.images->setChecked(types.contains('i'));
	d->ui.audios->setChecked(types.contains('a'));
	connect(d->ui.get, &QAbstractButton::clicked, this, &OpenMediaFolderDialog::getFolder);
	connect(d->ui.videos, &QAbstractButton::toggled, this, &OpenMediaFolderDialog::checkList);
	connect(d->ui.audios, &QAbstractButton::toggled, this, &OpenMediaFolderDialog::checkList);
	connect(d->ui.images, &QAbstractButton::toggled, this, &OpenMediaFolderDialog::checkList);
	connect(d->ui.list, &QListWidget::itemChanged, this, &OpenMediaFolderDialog::updateOpenButton);
	d->ui.dbb->button(QDialogButtonBox::Open)->setEnabled(false);
	adjustSize();
}

OpenMediaFolderDialog::~OpenMediaFolderDialog() {
	delete d;
}

void OpenMediaFolderDialog::updateOpenButton() {
	if (!d->generating) {
		d->ui.dbb->button(QDialogButtonBox::Open)->setEnabled(false);
		for (int i=0; i<d->ui.list->count(); ++i) {
			if (d->ui.list->item(i)->checkState() == Qt::Checked) {
				d->ui.dbb->button(QDialogButtonBox::Open)->setEnabled(true);
				break;
			}
		}
	}
}

void OpenMediaFolderDialog::setFolder(const QString &folder) {
	if (d->ui.folder->text() != folder) {
		d->ui.folder->setText(folder);
		updateList();
	}
}

void OpenMediaFolderDialog::checkList(bool checked) {
	QCheckBox *box = qobject_cast<QCheckBox*>(sender());
	for (int i=0; i<d->ui.list->count(); ++i) {
		auto item = d->ui.list->item(i);
		if (item->data(Type).value<QCheckBox*>() == box)
			item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	}
	QString types;
	if (d->ui.videos->isChecked())
		types.append('v');
	if (d->ui.audios->isChecked())
		types.append('a');
	if (d->ui.images->isChecked())
		types.append('i');
	AppState::get().open_folder_types = types;
}

void OpenMediaFolderDialog::updateList() {
	const auto folder = d->ui.folder->text();
	if (!folder.isEmpty()) {
		d->ui.list->clear();
		QDir dir(folder);
		static const auto filters = Info::mediaNameFilter();
		const QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
		for (int i=0; i<files.size(); ++i) {
			auto item = new QListWidgetItem(files[i].fileName(), d->ui.list);
			QCheckBox *box = nullptr;
			item->setCheckState(Qt::Unchecked);
			const QString suffix = files[i].suffix();
			if (Info::videoExt().contains(suffix, Qt::CaseInsensitive))
				box = d->ui.videos;
			else if (Info::videoExt().contains(suffix, Qt::CaseInsensitive))
				box = d->ui.audios;
			else if (Info::readableImageExt().contains(suffix, Qt::CaseInsensitive))
				box = d->ui.images;
			Q_ASSERT(box);
			item->setIcon(d->icons.icon(files[i]));
			item->setData(Type, QVariant::fromValue(box));
			item->setData(Path, files[i].absoluteFilePath());
			item->setCheckState(box->isChecked() ? Qt::Checked : Qt::Unchecked);
		}
		updateOpenButton();
	}
}

Playlist OpenMediaFolderDialog::playlist() const {
	Playlist list;
	for (int i=0; i<d->ui.list->count(); ++i) {
		const auto item = d->ui.list->item(i);
		if (item->checkState() == Qt::Checked)
			list.append(item->data(Path).toString());
	}
	return list;
}

void OpenMediaFolderDialog::getFolder() {
	AppState &as = AppState::get();
	const QString dir = QFileInfo(as.open_last_folder).absolutePath();
	const QString folder = QFileDialog::getExistingDirectory(this, tr("Open Folder"),  dir);
	if (!folder.isEmpty()) {
		as.open_last_folder = folder;
		setFolder(folder);
	}
}

int OpenMediaFolderDialog::exec() {
	getFolder();
	if (d->ui.folder->text().isEmpty())
		return Rejected;
	return QDialog::exec();

}
