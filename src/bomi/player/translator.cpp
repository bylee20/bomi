#include "translator.hpp"
#include "configure.hpp"
#include "misc/log.hpp"
#include "misc/encodinginfo.hpp"
#include <QTranslator>
#include <QSet>
#include <QLibraryInfo>

DECLARE_LOG_CONTEXT(Translator)

auto translator_load(const Locale &locale) -> bool
{
    return Translator::load(locale);
}

auto translator_default_encoding() -> EncodingInfo
{
    return Translator::defaultEncoding();
}

struct Iso639_2 { QString b, t; };

struct Translator::Data {
    bool succ = false;
    QTranslator trans, qt;
    QString path, def, file;
    QStringList dirs;
    QSet<Locale> locales;
    auto tryLoad(QTranslator *tr, const QString &file) -> bool
    {
        for (auto &dir : dirs) {
            if (!dir.isEmpty() && tr->load(file, dir))
                return true;
        }
        return false;
    }
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
    auto push = [&] (const QString &dir)
        { if (!dir.isEmpty()) d->dirs.push_back(dir); };
    push(QString::fromLocal8Bit(qgetenv("BOMI_TRANSLATION_PATH")));
    push(qApp->applicationDirPath() % "/translations"_a);
    push(QDir::homePath() % "/.bomi/translations"_a);
    push(u"" BOMI_TRANSLATIONS_PATH ""_q);

    qApp->installTranslator(&d->trans);
    qApp->installTranslator(&d->qt);
    for (auto &dir : d->dirs)
        d->locales += getLocales(dir, u"*.qm"_q, u"(.*).qm"_q);
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
    if (l.language() == QLocale::English) {
        d->tryLoad(&d->trans, file);
        d->succ = true;
    } else
        d->succ = d->tryLoad(&d->trans, file);
    if (d->succ) {
        d->tryLoad(&d->qt, "qt_"_a % l.name());
        Locale::setNative(l);
    } else
        _Warn("Failed to load translation file: %%", file, d->def);
    return d->succ;
}

auto Translator::defaultEncoding() -> EncodingInfo
{
    const auto name = tr("UTF-8",
        "Specify most popular encoding here in target localization.");
    const auto e = EncodingInfo::fromName(name);
    return e.isValid() ? e : EncodingInfo::fromName(u"UTF-8"_q);
}
