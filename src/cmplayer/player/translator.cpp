#include "translator.hpp"
#include "misc/log.hpp"
#include <unicode/locid.h>

DECLARE_LOG_CONTEXT(Translator)

auto translator_load(const QLocale &locale) -> bool
{
    return Translator::load(locale);
}

auto translator_display_language(const QString &iso) -> QString
{
    return Translator::displayLanguage(iso);
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
    QSet<QLocale> locales;
    icu::Locale icu;
    QMap<QString, QString> langs;
    QHash<QString, QString> b2t;
};

SIA qHash(const QLocale &key) -> uint
{
    return qHash(key.name());
}

auto getLocales(const QString &path, const QString &filter,
                const QString &regExp) -> QSet<QLocale>
{
    const QDir dir(path);
    const QStringList files = dir.entryList(QStringList(filter));
    QRegEx rx(u"^cmplayer_"_q % regExp % '$'_q);
    QSet<QLocale> set;
    for (auto &file : files) {
        const auto match = rx.match(file);
        if (match.hasMatch())
            set.insert(QLocale(match.captured(1)));
    }
    return set;
}

Translator::Translator()
: d(new Data) {
    d->def = u":/translations"_q;
    qApp->installTranslator(&d->trans);
    qApp->installTranslator(&d->qt);
    d->locales += getLocales(d->def, u"*.qm"_q, u"(.*).qm"_q);
    d->path = QString::fromLocal8Bit(qgetenv("CMPLAYER_TRANSLATION_PATH"));
    if (!d->path.isEmpty())
        d->locales += getLocales(d->path, u"*.qm"_q, u"(.*).qm"_q);

    d->b2t[u"cze"_q] = u"ces"_q;
    d->b2t[u"baq"_q] = u"eus"_q;
    d->b2t[u"fre"_q] = u"fra"_q;
    d->b2t[u"ger"_q] = u"deu"_q;
    d->b2t[u"gre"_q] = u"ell"_q;
    d->b2t[u"arm"_q] = u"hye"_q;
    d->b2t[u"ice"_q] = u"isl"_q;
    d->b2t[u"geo"_q] = u"kat"_q;
    d->b2t[u"mac"_q] = u"mkd"_q;
    d->b2t[u"mao"_q] = u"mri"_q;
    d->b2t[u"may"_q] = u"msa"_q;
    d->b2t[u"bur"_q] = u"mya"_q;
    d->b2t[u"dut"_q] = u"nld"_q;
    d->b2t[u"per"_q] = u"fas"_q;
    d->b2t[u"rum"_q] = u"ron"_q;
    d->b2t[u"slo"_q] = u"slk"_q;
    d->b2t[u"alb"_q] = u"sqi"_q;
    d->b2t[u"tib"_q] = u"bod"_q;
    d->b2t[u"wel"_q] = u"cym"_q;
    d->b2t[u"chi"_q] = u"zho"_q;

    // custom code for opensubtitles
    d->b2t[u"scc"_q] = u"srp"_q;
    d->b2t[u"pob"_q] = u"por"_q;
    d->b2t[u"pb"_q]  = u"pt"_q;
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

auto Translator::load(const QLocale &locale) -> bool
{
    QLocale l = locale;
    if (locale.language() == QLocale::C)
        l = QLocale("en_US"_a);
    const QString file = "cmplayer_"_a % l.name();
    Translator::Data *d = get().d;
    if (file == d->file)
        return d->succ;
    d->icu = icu::Locale::createFromName(l.name().toLatin1().data());
    d->langs.clear();
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
        QLocale::setDefault(l);
    }
    return d->succ;
}

auto Translator::defaultEncoding() -> QString
{
    auto enc = tr("UTF-8",
                  "Specify most popular encoding here in target localization.");
    return enc.isEmpty() ? u"UTF-8"_q : enc;
}

auto Translator::displayLanguage(const QString &_iso) -> QString
{
    auto d = get().d;
    QString iso = _iso.toLower();
    auto it = d->b2t.constFind(iso);
    if (it != d->b2t.cend())
        iso = *it;
    auto &name = d->langs[iso];
    if (name.isEmpty()) {
        icu::UnicodeString str;
        icu::Locale locale(iso.toLatin1());
        locale.getDisplayLanguage(d->icu, str);
        name.setUtf16(str.getBuffer(), str.length());
        if (iso == name)
            _Error("Cannot find locale for %%", iso);
    }
    return name;
}

auto Translator::displayName(const QLocale &locale) -> QString
{
    auto d = get().d;
    auto l = ::icu::Locale::createFromName(locale.name().toLatin1().data());
    icu::UnicodeString str;
    l.getDisplayName(d->icu, str);
    return QString::fromUtf16(str.getBuffer(), str.length());
}

