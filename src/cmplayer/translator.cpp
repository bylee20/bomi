#include "translator.hpp"
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QDir>
#include <QtCore/QSet>

struct Translator::Data {
	bool succ = false;
	QTranslator trans;
	QString path, def, file;
	QSet<QLocale> locales;
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
	d->file = file;
	d->succ = (d->trans.load(file, d->path) || d->trans.load(file, d->def));
	if (d->succ)
		QLocale::setDefault(l);
	return d->succ;
}
