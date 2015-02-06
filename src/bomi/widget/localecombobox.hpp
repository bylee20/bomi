#ifndef LOCALECOMBOBOX_HPP
#define LOCALECOMBOBOX_HPP

#include "misc/locale.hpp"

class LocaleComboBox : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(Locale locale READ currentLocale WRITE setCurrentLocale NOTIFY currentLocaleChanged)
public:
    LocaleComboBox(QWidget *parent = nullptr);
    ~LocaleComboBox();
    auto currentLocale() const -> Locale;
    auto setCurrentLocale(const Locale &locale) -> void;
signals:
    void currentLocaleChanged();
private:
    auto changeEvent(QEvent *event) -> void;
    struct Item;
    struct Data;
    Data *d;
};

#endif // LOCALECOMBOBOX_HPP
