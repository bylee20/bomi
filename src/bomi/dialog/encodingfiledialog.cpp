#include "encodingfiledialog.hpp"
#include "widget/encodingcombobox.hpp"

EncodingFileDialog::EncodingFileDialog(QWidget *parent, const QString &caption
        , const QString &directory, const QString &filter, const EncodingInfo &encoding)
: QFileDialog(parent, caption, directory, filter), combo(new EncodingComboBox(this)) {
    auto grid = qobject_cast<QGridLayout*>(layout());
    if (grid) {
        const int row = grid->rowCount();
        grid->addWidget(new QLabel(tr("Encoding") % ':'_q, this), row, 0, 1, 1);
        grid->addWidget(combo, row, 1, 1, grid->columnCount()-1);
    }
    if (encoding.isValid())
        setEncoding(encoding);
}

auto EncodingFileDialog::setEncoding(const EncodingInfo &encoding) -> void
{
    combo->setEncoding(encoding);
}

auto EncodingFileDialog::encoding() const -> EncodingInfo
{
    return combo->encoding();
}

auto EncodingFileDialog::getOpenFileName(QWidget *parent, const QString &caption,
                                         const QString &dir, const QString &filter,
                                         EncodingInfo *enc) -> QString
{
    const auto files = getOpenFileNames(parent, caption, dir, filter, enc, ExistingFile);
    return files.isEmpty() ? QString() : files[0];
}

auto EncodingFileDialog::getOpenFileNames(QWidget *parent, const QString &caption,
                                          const QString &dir, const QString &filter,
                                          EncodingInfo *enc, FileMode mode) -> QStringList
{
    EncodingFileDialog dlg(parent, caption, dir, filter);
    if (enc && enc->isValid())
        dlg.setEncoding(*enc);
    dlg.setFileMode(mode);
    if (dlg.exec()) {
        if (enc)
            *enc = dlg.encoding();
        return dlg.selectedFiles();
    }
    return QStringList();
}
