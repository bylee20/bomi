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
    connect(d->ui.folder, &QPushButton::clicked, this, [this] () {
        auto dir = _GetOpenDir(this, tr("Open device or folder"));
        if (!dir.isEmpty())
            d->ui.device->setCurrentText(dir);
    });
    connect(d->ui.iso, &QPushButton::clicked, this, [this] () {
        auto iso = _GetOpenFile(this, tr("Open ISO file"), DiscExt);
        if (!iso.isEmpty())
            d->ui.device->setCurrentText(iso);
    });
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
        : _L("<font color='red'>") % tr("Selected device doesn't exists.") % _L("</font>"));
}

auto OpenDiscDialog::device() const -> QString
{
    return d->ui.device->currentText();
}

auto OpenDiscDialog::setDevice(const QString &device) -> void
{
    d->ui.device->setCurrentText(device);
}
