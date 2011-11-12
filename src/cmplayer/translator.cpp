#include "translator.hpp"
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QDir>
#include <QtCore/QSet>

struct Translator::Data {
	QTranslator trans;
	QString path;
	QSet<QLocale> locale;
	QString def;
};

static inline uint qHash(const QLocale &key) {
	return qHash(key.name());
}

QSet<QLocale> getLocales(const QString &path, const QString &filter, const QString &regExp) {
	const QDir dir(path);
	const QStringList file = dir.entryList(QStringList(filter));
	QRegExp rx("^cmplayer_" + regExp + "$");
	QSet<QLocale> set;
	for (int i=0; i<file.size(); ++i) {
		if (rx.indexIn(file[i]) == -1)
			continue;
		set.insert(QLocale(rx.cap(1)));
	}
	return set;
}

Translator::Translator()
: d(new Data) {
	d->def = ":/translations";
	qApp->installTranslator(&d->trans);
	d->locale += getLocales(d->def, "*", "(.*)");
	d->path = QString::fromLocal8Bit(qgetenv("CMPLAYER_TRANSLATION_PATH"));
	if (!d->path.isEmpty())
		d->locale += getLocales(d->path, "*.qm", "(.*).qm");
}

Translator::~Translator() {
	delete d;
}

Translator &Translator::get() {
	static Translator self;
	return self;
}

LocaleList Translator::availableLocales() {
	QList<QLocale> list = get().d->locale.toList();
	list.prepend(QLocale::c());
	return list;
}

bool Translator::load(const QLocale &locale) {
	QLocale l = locale;
	if (locale.language() == QLocale::C)
		l = QLocale::system();
	const QString file = "cmplayer_" + l.name();
	Translator::Data *d = get().d;
	const bool ret = (d->trans.load(file, d->path) || d->trans.load(file, d->def));
	if (ret)
		QLocale::setDefault(l);
	return ret;
}
