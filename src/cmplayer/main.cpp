#include "app.hpp"
#include <QtCore/QDebug>
#include "pref.hpp"

int main(int argc, char **argv) {
	App app(argc, argv);
	Pref pref;
	Pref::obj = &pref;
	const int ret = app.exec();
	return ret;
}
