#include "encodingcombobox.hpp"

EncodingComboBox::EncodingComboBox(QWidget *parent)
: DataComboBox(parent) {
    auto add = [this] (const QString &desc, const QString &enc)
        { this->addItem(desc % " ("_a % enc % ')'_q, enc); };
    add(u"UTF-8"_q, u"UTF-8"_q);
    add(u"Unicode"_q, u"Unicode"_q);
    add(u"Western European Languages"_q, u"CP1252"_q);
    add(u"Western European Languages With Euro"_q, u"ISO-8859-15"_q);
    add(u"Slavic/Central European Languages"_q, u"ISO-8859-2"_q);
    add(u"Slavic/Central European Windows"_q, u"CP1250"_q);
    add(u"Esperanto, Galician, Maltese, Turkish"_q, u"ISO-8859-3"_q);
    add(u"Old Baltic Charset"_q, u"ISO-8859-4"_q);
    add(u"Cyrillic"_q, u"ISO-8859-5"_q);
    add(u"Cyrillic Windows"_q, u"CP1251"_q);
    add(u"Arabic"_q, u"ISO-8859-6"_q);
    add(u"Modern Greek"_q, u"ISO-8859-7"_q);
    add(u"Turkish"_q, u"ISO-8859-9"_q);
    add(u"Baltic"_q, u"ISO-8859-13"_q);
    add(u"Celtic"_q, u"ISO-8859-14"_q);
    add(u"Hebrew Charset"_q, u"ISO-8859-8"_q);
    add(u"Russian"_q, u"KOI8-R"_q);
    add(u"Ukrainian, Belarusian"_q, u"KOI8-U/RU"_q);
    add(u"Simplified Chinese Charset"_q, u"CP936"_q);
    add(u"Traditional Chinese Charset"_q, u"BIG5"_q);
    add(u"Chinese Government Standard Charset"_q, u"GB18030"_q);
    add(u"Japanese Charset"_q, u"SHIFT-JIS"_q);
    add(u"Japanese Charset"_q, u"ISO 2022-JP"_q);
    add(u"Japanese Charset"_q, u"EUC-JP"_q);
    add(u"Korean Charset"_q, u"CP949"_q);
    add(u"Thai Charset"_q, u"CP874"_q);
    connect(this, &QComboBox::currentTextChanged,
            this, &EncodingComboBox::encodingChanged);
}

auto EncodingComboBox::encoding() const -> QString
{
    return currentValue<QString>();
}

auto EncodingComboBox::setEncoding(const QString &encoding) -> void
{
    setCurrentValue<QString>(encoding.toUpper());
}
