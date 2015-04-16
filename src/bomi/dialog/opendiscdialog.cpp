#include "opendiscdialog.hpp"
#include "ui_opendvddialog.h"

struct OpenDiscDialog::Data {
    Ui::OpenDiscDialog ui;
    QPushButton *ok;
};

OpenDiscDialog::OpenDiscDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->ui.setupUi(this);
    d->ok = d->ui.buttonBox->button(QDialogButtonBox::Ok);
    d->ok->setEnabled(false);
    connect(d->ui.device, &QComboBox::editTextChanged, this, &OpenDiscDialog::checkDevice);
    checkDevice(d->ui.device->currentText());
    d->ui.folder->setEditor(d->ui.device);
    d->ui.iso->setEditor(d->ui.device);
    d->ui.iso->setMode(PathButton::SingleFile);
    d->ui.iso->setFilter(DiscExt);
}

OpenDiscDialog::~OpenDiscDialog() {
    delete d;
}

auto OpenDiscDialog::setIsoEnabled(bool on) -> void
{
    d->ui.iso->setVisible(on);
}

auto OpenDiscDialog::setDeviceList(const QStringList &devices) -> void
{
    d->ui.device->clear();
    d->ui.device->addItems(devices);
}

auto OpenDiscDialog::checkDevice(const QString &device) -> void
{
    const QFileInfo info(device);
    const bool exists = info.exists();
    d->ok->setEnabled(exists);
    d->ui.available->setText(exists ? tr("Selected device is available.")
        : "<font color='red'>"_a % tr("Selected device doesn't exists.") % "</font>"_a);
}

auto OpenDiscDialog::device() const -> QString
{
    return d->ui.device->currentText();
}

auto OpenDiscDialog::setDevice(const QString &device) -> void
{
    d->ui.device->setCurrentText(device);
}
