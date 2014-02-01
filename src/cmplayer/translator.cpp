#include "translator.hpp"
#include <unicode/locid.h>

struct Translator::Data {
	bool succ = false;
	QTranslator trans;
	QString path, def, file;
	QSet<QLocale> locales;
	icu::Locale icu;
	QMap<QString, QString> langs;
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
	d->locales += getLocales(d->def, "*.qm", "(.*).qm");
	d->path = QString::fromLocal8Bit(qgetenv("CMPLAYER_TRANSLATION_PATH"));
	if (!d->path.isEmpty())
		d->locales += getLocales(d->path, "*.qm", "(.*).qm");
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
	QList<QLocale> list = get().d->locales.toList();
	list.prepend(QLocale::c());
	return list;
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
	if ((d->succ = (d->trans.load(file, d->path) || d->trans.load(file, d->def))))
		QLocale::setDefault(l);
	return d->succ;
}

QString Translator::defaultEncoding() {
	return tr("UTF-8", "Specify most popular encoding here in target localization.");
}

QString Translator::displayLanguage(const QString &iso) {
	auto d = get().d;
	auto &name = d->langs[iso];
	if (name.isEmpty()) {
		icu::UnicodeString str;
		icu::Locale locale(iso.toLatin1());
		locale.getDisplayLanguage(d->icu, str);
		name.setUtf16(str.getBuffer(), str.length());
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

