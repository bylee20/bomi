#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

using LocaleList = QList<QLocale>;

class Translator : public QObject {
    Q_OBJECT
public:
    ~Translator();
    static auto load(const QLocale &locale = QLocale::system()) -> bool;
    static auto availableLocales() -> LocaleList;
    static auto defaultEncoding() -> QString;
    static auto displayLanguage(const QString &iso) -> QString;
    static auto displayName(const QLocale &locale) -> QString;
private:
    Translator();
    static Translator &get();
    struct Data;
    Data *d;
};

#endif // TRANSLATOR_HPP
