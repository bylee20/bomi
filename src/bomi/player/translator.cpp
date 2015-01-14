#include "translator.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(Translator)

auto translator_load(const Locale &locale) -> bool
{
    return Translator::load(locale);
}

auto translator_default_encoding() -> QString
{
    return Translator::defaultEncoding();
}

struct Iso639_2 { QString b, t; };

struct Translator::Data {
    bool succ = false;
    QTranslator trans, qt;
    QString path, def, file;
    QSet<Locale> locales;
};

SIA qHash(const Locale &key) -> uint
{
    return qHash(key.name());
}

auto getLocales(const QString &path, const QString &filter,
                const QString &regExp) -> QSet<Locale>
{
    const QDir dir(path);
    const QStringList files = dir.entryList(QStringList(filter));
    QRegEx rx(u"^bomi_"_q % regExp % '$'_q);
    QSet<Locale> set;
    for (auto &file : files) {
        const auto match = rx.match(file);
        if (match.hasMatch())
            set.insert(Locale(match.captured(1)));
    }
    return set;
}

Translator::Translator()
: d(new Data) {
    d->def = u":/translations"_q;
    qApp->installTranslator(&d->trans);
    qApp->installTranslator(&d->qt);
    d->locales += getLocales(d->def, u"*.qm"_q, u"(.*).qm"_q);
    d->path = QString::fromLocal8Bit(qgetenv("BOMI_TRANSLATION_PATH"));
    if (!d->path.isEmpty())
        d->locales += getLocales(d->path, u"*.qm"_q, u"(.*).qm"_q);


}

Translator::~Translator() {
    delete d;
}

auto Translator::get() -> Translator&
{
    static Translator self;
    return self;
}

auto Translator::availableLocales() -> LocaleList
{
    LocaleList list;
    for (auto &locale : get().d->locales)
        list.push_back(locale);
    return list;
}

auto Translator::load(const Locale &locale) -> bool
{
    Locale l = locale;
    if (!locale.isValid())
        l = Locale::system();
    Q_ASSERT(l.isValid());
    if (l.language() == QLocale::C)
        l = Locale("en_US"_a);
    const QString file = "bomi_"_a % l.name();
    Translator::Data *d = get().d;
    if (file == d->file)
        return d->succ;
    d->file = file;
    auto loadQt = [&] () {
        auto path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString qm = "qt_"_a % l.name();
        if (path.isEmpty() || !d->qt.load(qm, path))
            _Error("Cannot find translations for Qt, %% in %%", qm, path);
    };

    if (l.language() == QLocale::English) {
        d->succ = (d->trans.load(file, d->path) || d->trans.load(file, d->def));
        d->succ = true;
    } else
        d->succ = (d->trans.load(file, d->path) || d->trans.load(file, d->def));
    if (d->succ) {
        loadQt();
        Locale::setNative(l);
    }
    return d->succ;
}

auto Translator::defaultEncoding() -> QString
{
    auto enc = tr("UTF-8",
                  "Specify most popular encoding here in target localization.");
    return enc.isEmpty() ? u"UTF-8"_q : enc;
}
