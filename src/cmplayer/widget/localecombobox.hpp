#ifndef LOCALECOMBOBOX_HPP
#define LOCALECOMBOBOX_HPP

#include "stdafx.hpp"

class LocaleComboBox : public QComboBox {
    Q_OBJECT
public:
    LocaleComboBox(QWidget *parent = nullptr);
    ~LocaleComboBox();
    auto currentLocale() const -> QLocale
        { return itemData(currentIndex()).toLocale(); }
    auto setCurrentLocale(const QLocale &locale) -> void
        { setCurrentIndex(findData(locale)); }
private:
    auto changeEvent(QEvent *event) -> void;
    struct Item;
    struct Data;
    Data *d;
};

#endif // LOCALECOMBOBOX_HPP
