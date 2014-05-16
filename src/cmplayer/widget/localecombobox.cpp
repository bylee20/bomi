#include "localecombobox.hpp"
#include "translator.hpp"

struct LocaleComboBox::Item {
    bool operator == (const Item &rhs) const { return locale == rhs.locale; }
    bool operator < (const Item &rhs) const { return name < rhs.name; }
    Item(const QLocale &locale): locale(locale) { }
    void update() { name = Translator::displayName(locale); }
    QString name;
    QLocale locale;
};

struct LocaleComboBox::Data {
    LocaleComboBox *p = nullptr;
    QList<Item> items;
    Item system{QLocale::system()};
    auto reset() -> void
    {
        auto locale = p->currentLocale();
        p->clear();
        for (auto &it : items)
            it.update();
        qSort(items);
        system.update();
        p->addItem(tr("System locale[%1]").arg(system.name), system.locale);
        for (auto &it : items)
            p->addItem(it.name, it.locale);
        p->setCurrentLocale(locale);
    }
};

LocaleComboBox::LocaleComboBox(QWidget *parent)
    : QComboBox(parent)
    , d(new Data)
{
    d->p = this;
    auto locales = Translator::availableLocales();
    d->items.reserve(locales.size());
    for (auto &locale : locales)
        d->items.append(Item{locale});
    d->reset();
}

LocaleComboBox::~LocaleComboBox()
{
    delete d;
}

auto LocaleComboBox::changeEvent(QEvent *event) -> void
{
    QComboBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        d->reset();
}
