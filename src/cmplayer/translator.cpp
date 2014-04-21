#include "translator.hpp"
#include <unicode/locid.h>
#include "log.hpp"

DECLARE_LOG_CONTEXT(Translator)

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

static inline uint qHash(const QLocale &key) {
    return qHash(key.name());
}

QSet<QLocale> getLocales(const QString &path, const QString &filter, const QString &regExp) {
    const QDir dir(path);
    const QStringList files = dir.entryList(QStringList(filter));
    QRegExp rx("^cmplayer_" + regExp + "$");
    QSet<QLocale> set;
    for (auto &file : files) {
        if (rx.indexIn(file) != -1)
            set.insert(QLocale(rx.cap(1)));
    }
    return set;
}

Translator::Translator()
: d(new Data) {
    d->def = ":/translations";
    qApp->installTranslator(&d->trans);
    qApp->installTranslator(&d->qt);
    d->locales += getLocales(d->def, "*.qm", "(.*).qm");
    d->path = QString::fromLocal8Bit(qgetenv("CMPLAYER_TRANSLATION_PATH"));
    if (!d->path.isEmpty())
        d->locales += getLocales(d->path, "*.qm", "(.*).qm");

    d->b2t[_L("cze")] = _L("ces");
    d->b2t[_L("baq")] = _L("eus");
    d->b2t[_L("fre")] = _L("fra");
    d->b2t[_L("ger")] = _L("deu");
    d->b2t[_L("gre")] = _L("ell");
    d->b2t[_L("arm")] = _L("hye");
    d->b2t[_L("ice")] = _L("isl");
    d->b2t[_L("geo")] = _L("kat");
    d->b2t[_L("mac")] = _L("mkd");
    d->b2t[_L("mao")] = _L("mri");
    d->b2t[_L("may")] = _L("msa");
    d->b2t[_L("bur")] = _L("mya");
    d->b2t[_L("dut")] = _L("nld");
    d->b2t[_L("per")] = _L("fas");
    d->b2t[_L("rum")] = _L("ron");
    d->b2t[_L("slo")] = _L("slk");
    d->b2t[_L("alb")] = _L("sqi");
    d->b2t[_L("tib")] = _L("bod");
    d->b2t[_L("wel")] = _L("cym");
    d->b2t[_L("chi")] = _L("zho");

    // custom code for opensubtitles
    d->b2t[_L("scc")] = _L("srp");
    d->b2t[_L("pob")] = _L("por");
    d->b2t[_L("pb")] = _L("pt");
}

Translator::~Translator() {
    delete d;
}

Translator &Translator::get() {
    static Translator self;
    return self;
}

LocaleList Translator::availableLocales() {
    return get().d->locales.toList();
}

bool Translator::load(const QLocale &locale) {
    QLocale l = locale;
    if (locale.language() == QLocale::C)
        l = QLocale::system();
    const QString file = "cmplayer_" + l.name();
    Translator::Data *d = get().d;
    if (file == d->file)
        return d->succ;
    d->icu = icu::Locale::createFromName(l.name().toLatin1().data());
    d->langs.clear();
    d->file = file;
    if ((d->succ = (d->trans.load(file, d->path) || d->trans.load(file, d->def)))) {
        const auto path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString qm = _L("qt_") % l.name();
        if (path.isEmpty() || !d->qt.load(qm, path))
            _Error("Cannot find translations for Qt, %% in %%", qm, path);
        QLocale::setDefault(l);
    }
    return d->succ;
}

QString Translator::defaultEncoding() {
    auto enc = tr("UTF-8", "Specify most popular encoding here in target localization.");
    return enc.isEmpty() ? _L("UTF-8") : enc;
}

QString Translator::displayLanguage(const QString &_iso) {
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

QString Translator::displayName(const QLocale &locale) {
    auto d = get().d;
    auto l = ::icu::Locale::createFromName(locale.name().toLatin1().data());
    icu::UnicodeString str;
    l.getDisplayName(d->icu, str);
    return QString::fromUtf16(str.getBuffer(), str.length());
}

