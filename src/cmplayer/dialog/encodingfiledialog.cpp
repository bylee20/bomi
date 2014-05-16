#include "encodingfiledialog.hpp"
#include "widget/encodingcombobox.hpp"

EncodingFileDialog::EncodingFileDialog(QWidget *parent, const QString &caption
        , const QString &directory, const QString &filter, const QString &encoding)
: QFileDialog(parent, caption, directory, filter), combo(new EncodingComboBox(this)) {
    auto grid = qobject_cast<QGridLayout*>(layout());
    if (grid) {
        const int row = grid->rowCount();
        grid->addWidget(new QLabel(tr("Encoding") % ':', this), row, 0, 1, 1);
        grid->addWidget(combo, row, 1, 1, grid->columnCount()-1);
    }
    if (!encoding.isEmpty())
        setEncoding(encoding);
}

auto EncodingFileDialog::setEncoding(const QString &encoding) -> void
{
    combo->setEncoding(encoding);
}

auto EncodingFileDialog::encoding() const -> QString
{
    return combo->encoding();
}

QString EncodingFileDialog::getOpenFileName(QWidget *parent
        , const QString &caption, const QString &dir, const QString &filter, QString *enc) {
    const auto files = getOpenFileNames(parent, caption, dir, filter, enc, ExistingFile);
    return files.isEmpty() ? QString() : files[0];
}

QStringList EncodingFileDialog::getOpenFileNames(QWidget *parent
        , const QString &caption, const QString &dir, const QString &filter, QString *enc, FileMode mode) {
    EncodingFileDialog dlg(parent, caption, dir, filter);
    if (enc && !enc->isEmpty())
        dlg.setEncoding(*enc);
    dlg.setFileMode(mode);
    if (dlg.exec()) {
        if (enc)
            *enc = dlg.encoding();
        return dlg.selectedFiles();
    }
    return QStringList();
}
