#include "localecombobox.hpp"
#include "player/translator.hpp"

struct LocaleComboBox::Item {
    bool operator == (const Item &rhs) const { return locale == rhs.locale; }
    bool operator < (const Item &rhs) const { return name < rhs.name; }
    Item() { }
    Item(const Locale &locale): locale(locale) { }
    void update() { name = locale.nativeName(); }
    QString name;
    Locale locale;
};

struct LocaleComboBox::Data {
    LocaleComboBox *p = nullptr;
    QList<Item> items;
    Item system;
    auto reset() -> void
    {
        auto locale = p->currentLocale();
        p->clear();
        for (auto &it : items)
            it.update();
        qSort(items);
        auto s = Locale::system();
        p->addItem(tr("System locale[%1]").arg(s.nativeName()),
                   system.locale.toVariant());
        for (auto &it : items)
            p->addItem(it.name, it.locale.toVariant());
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
    auto signal = &LocaleComboBox::currentLocaleChanged;
    PLUG_CHANGED(this);
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

auto LocaleComboBox::currentLocale() const -> Locale
{
    return Locale::fromVariant(itemData(currentIndex()));
}

auto LocaleComboBox::setCurrentLocale(const Locale &locale) -> void
{
    setCurrentIndex(findData(locale.toVariant()));
}
