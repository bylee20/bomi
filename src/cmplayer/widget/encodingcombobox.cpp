#include "encodingcombobox.hpp"

EncodingComboBox::EncodingComboBox(QWidget *parent)
: QComboBox(parent) {
    enc = QStringList() << "UTF-8" << "Unicode"
            << tr("Western European Languages") + " (CP1252)"
            << tr("Western European Languages With Euro") + " (ISO-8859-15)"
            << tr("Slavic/Central European Languages") + " (ISO-8859-2)"
            << tr("Slavic/Central European Windows") + " (CP1250)"
            << tr("Esperanto, Galician, Maltese, Turkish") + " (ISO-8859-3)"
            << tr("Old Baltic Charset") + " (ISO-8859-4)"
            << tr("Cyrillic") + " (ISO-8859-5)"
            << tr("Cyrillic Windows") + " (CP1251)"
            << tr("Arabic") + " (ISO-8859-6)"
            << tr("Modern Greek") + " (ISO-8859-7)"
            << tr("Turkish") + " (ISO-8859-9)"
            << tr("Baltic") + " (ISO-8859-13)"
            << tr("Celtic") + " (ISO-8859-14)"
            << tr("Hebrew Charset") + " (ISO-8859-8)"
            << tr("Russian") + " (KOI8-R)"
            << tr("Ukrainian, Belarusian") + " (KOI8-U/RU)"
            << tr("Simplified Chinese Charset") + " (CP936)"
            << tr("Traditional Chinese Charset") + " (BIG5)"
            << tr("Japanese Charset") + " (SHIFT-JIS)"
            << tr("Korean Charset") + " (CP949)"
            << tr("Thai Charset") + " (CP874)";
    addItems(enc);
    setEditable(true);
}

auto EncodingComboBox::encoding() const -> QString
{
    QString enc = currentText().trimmed();
    QRegExp rxEnc(".* \\((.*)\\)");
    return (rxEnc.indexIn(enc) == -1) ? enc : rxEnc.cap(1);
}

auto EncodingComboBox::setEncoding(const QString &encoding) -> void
{
    static const QRegExp rxEncoding(".* \\(" + encoding.toUpper() + "\\)");
    const int idx = enc.indexOf(rxEncoding);
    if (idx != -1)
        setCurrentIndex(idx);
    else
        setEditText(encoding);
}
