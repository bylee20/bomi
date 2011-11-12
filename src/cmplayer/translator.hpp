#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QLocale>

typedef QList<QLocale> LocaleList;

class Translator : public QObject {
	Q_OBJECT
public:
	~Translator();
	static bool load(const QLocale &locale = QLocale::system());
	static LocaleList availableLocales();
private:
	Translator();
	static Translator &get();
	struct Data;
	Data *d;
};

#endif // TRANSLATOR_HPP
