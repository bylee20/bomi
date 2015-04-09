#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include "misc/locale.hpp"

using LocaleList = QVector<Locale>;     class EncodingInfo;

class Translator : public QObject {
    Q_DECLARE_TR_FUNCTIONS(Translator)
public:
    ~Translator();
    static auto load(const Locale &locale = Locale::system()) -> bool;
    static auto availableLocales() -> LocaleList;
    static auto defaultEncoding() -> EncodingInfo;
private:
    Translator();
    static Translator &get();
    struct Data;
    Data *d;
};

#endif // TRANSLATOR_HPP
