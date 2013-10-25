#ifndef SKIN_HPP
#define SKIN_HPP

#include "stdafx.hpp"

class Skin {
public:
	~Skin() {}
	static QStringList dirs() {return data()->dirs;}
	static QStringList names(bool reload = false);
	static QFileInfo source(const QString &name) {
		auto it = data()->skins.find(name);
		if (it != data()->skins.end())
			return it.value();
		return QFileInfo();
	}
	static void apply(QQuickView *view, const QString &name);
protected:
	Skin() {}
private:
	struct Data {
		Data();
		QStringList dirs, qmls;
		QMap<QString, QFileInfo> skins;
	};
	static Data *data() { static Data data;	return &data; }
};

#endif // SKIN_HPP
