#include "appobject.hpp"

void reg_app_object() {
	qmlRegisterSingletonType<AppObject>("CMPlayer", 1, 0, "App", _QmlSingleton<AppObject>);
}

AppObject::StaticData AppObject::s;
