#include "urldialog.hpp"
#include "bbox.hpp"
#include "player/playlist.hpp"
#include "player/appstate.hpp"
#include "widget/encodingcombobox.hpp"

struct UrlDialog::Data {
    UrlDialog *p;
    QComboBox *url;
    EncodingComboBox *enc;
    QCompleter *c;
    BBox *bbox;
    Playlist playlist;
};

UrlDialog::UrlDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    const auto &as = AppState::get();
    d->c = new QCompleter(as.open_url_list, this);
    d->url = new QComboBox(this);
    d->url->setEditable(true);
    d->url->addItems(as.open_url_list);
    d->url->setCompleter(d->c);
    d->url->setMaximumWidth(500);
    d->enc = new EncodingComboBox(this);
    d->enc->setEncoding(as.open_url_enc);
    d->bbox = BBox::make(this);

    auto form = new QFormLayout;
    form->addRow(tr("URL"), d->url);
    form->addRow(tr("Encoding for Playlist"), d->enc);

    auto vbox = new QVBoxLayout(this);
    vbox->addLayout(form);
    vbox->addWidget(d->bbox);

    setWindowTitle(tr("Open URL"));
    setMaximumWidth(700);
}

UrlDialog::~UrlDialog() {
    delete d;
}

auto UrlDialog::accept() -> void
{
    auto &as = AppState::get();
    const auto url = d->url->currentText().trimmed();
    const int idx = as.open_url_list.indexOf(url);
    if (idx >= 0)
        as.open_url_list.takeAt(idx);
    as.open_url_list.prepend(url);
    as.open_url_enc = d->enc->encoding();
    QDialog::accept();
}

auto UrlDialog::url() const -> QUrl
{
    return QUrl(d->url->currentText().trimmed());
}

auto UrlDialog::isPlaylist() const -> bool
{
    return _IsSuffixOf(PlaylistExt, QFileInfo(url().path()).suffix());
}

auto UrlDialog::playlist() const -> Playlist
{
    return d->playlist;
}

auto UrlDialog::encoding() const -> QString
{
    return d->enc->encoding();
}
