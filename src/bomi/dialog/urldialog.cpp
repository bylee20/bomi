#include "urldialog.hpp"
#include "bbox.hpp"
#include "misc/objectstorage.hpp"
#include "player/playlist.hpp"
#include "widget/encodingcombobox.hpp"
#include <QCompleter>

struct UrlDialog::Data {
    UrlDialog *p;
    QComboBox *url;
    EncodingComboBox *enc;
    QCompleter *c;
    BBox *bbox;
    QCheckBox *playlist = nullptr;
    ObjectStorage storage;
    QStringList urls;
    EncodingInfo encoding;
};

UrlDialog::UrlDialog(QWidget *parent, const QString &key)
: QDialog(parent), d(new Data) {
    d->p = this;

    d->storage.setObject(this, "UrlDialog-"_a % key);
    d->storage.add("open_url_list", &d->urls);
    d->storage.add("open_url_enc", &d->encoding);
    d->storage.restore();

    d->c = new QCompleter(d->urls, this);
    d->url = new QComboBox(this);
    d->url->setEditable(true);
    d->url->addItems(d->urls);
    d->url->setCompleter(d->c);
    d->url->setMaximumWidth(500);
    d->enc = new EncodingComboBox(this);
    d->enc->setEncoding(d->encoding);
    d->bbox = BBox::make(this);
    d->playlist = new QCheckBox(tr("Handle as playlist"), this);
    auto form = new QFormLayout;
    form->addRow(tr("URL"), d->url);
    form->addRow(tr("Encoding"), d->enc);

    auto hbox = new QHBoxLayout;
    hbox->addWidget(d->playlist);
    hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    hbox->addWidget(d->bbox);

    auto vbox = new QVBoxLayout(this);
    vbox->addLayout(form);
    vbox->addLayout(hbox);

    _SetWindowTitle(this, tr("Open URL"));
    setMaximumWidth(700);
}

UrlDialog::~UrlDialog() {
    d->storage.save();
    delete d;
}

auto UrlDialog::accept() -> void
{
    const auto url = d->url->currentText().trimmed();
    d->urls.clear();
    d->urls.reserve(1 + d->url->count());
    d->urls.append(url);
    for (int i = 0; i < d->url->count(); ++i) {
        const auto item = d->url->itemText(i);
        if (item != url)
            d->urls.append(item);
    }
    d->encoding = d->enc->encoding();
    d->storage.save();
    QDialog::accept();
}

auto UrlDialog::url() const -> QUrl
{
    return QUrl(d->url->currentText().trimmed());
}

auto UrlDialog::isPlaylist() const -> bool
{
    return d->playlist->isChecked();
}

auto UrlDialog::encoding() const -> EncodingInfo
{
    return d->enc->encoding();
}

auto UrlDialog::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    d->url->setFocus();
}
