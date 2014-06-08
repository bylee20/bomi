#include "urldialog.hpp"
#include "bbox.hpp"
#include "player/playlist.hpp"
#include "widget/encodingcombobox.hpp"

struct UrlDialog::Data {
    UrlDialog *p;
    QComboBox *url;
    EncodingComboBox *enc;
    QCompleter *c;
    BBox *bbox;
    QCheckBox *playlist = nullptr;
    QString key;
};
#define GROUP "UrlDialog_"_a

UrlDialog::UrlDialog(QWidget *parent, const QString &key)
: QDialog(parent), d(new Data) {
    d->p = this;

    d->key = key;
    QSettings settings;
    settings.beginGroup(GROUP % d->key);
    auto urls = settings.value(u"open_url_list"_q).toStringList();
    auto enc  = settings.value(u"open_url_enc"_q).toString();
    settings.endGroup();

    d->c = new QCompleter(urls, this);
    d->url = new QComboBox(this);
    d->url->setEditable(true);
    d->url->addItems(urls);
    d->url->setCompleter(d->c);
    d->url->setMaximumWidth(500);
    d->enc = new EncodingComboBox(this);
    d->enc->setEncoding(enc);
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

    setWindowTitle(tr("Open URL"));
    setMaximumWidth(700);
}

UrlDialog::~UrlDialog() {
    delete d;
}

auto UrlDialog::accept() -> void
{
    const auto url = d->url->currentText().trimmed();
    QStringList urls;
    urls.reserve(1 + d->url->count());
    urls.append(url);
    for (int i = 0; i < d->url->count(); ++i) {
        const auto item = d->url->itemText(i);
        if (item != url)
            urls.append(item);
    }
    QSettings settings;
    settings.beginGroup(GROUP % d->key);
    settings.setValue(u"open_url_list"_q, urls);
    settings.setValue(u"open_url_enc"_q, d->enc->encoding());
    settings.endGroup();
    QDialog::accept();
}

auto UrlDialog::url() const -> QUrl
{
    return QUrl(d->url->currentText().trimmed());
}

auto UrlDialog::isPlaylist() const -> bool
{
    return d->playlist->isChecked()
           && _IsSuffixOf(PlaylistExt, QFileInfo(url().path()).suffix());
}

auto UrlDialog::encoding() const -> QString
{
    return d->enc->encoding();
}
