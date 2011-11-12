#include "application.hpp"
#include <QtCore/QDebug>
#include "pref.hpp"

int main(int argc, char **argv) {
	Application app(argc, argv);
	Pref pref;
	Pref::obj = &pref;
	const int ret = app.exec();
	return ret;
}
