#include "appobject.hpp"

auto reg_app_object() -> void {
    qmlRegisterSingletonType<AppObject>("bomi", 1, 0, "App",
                                        _QmlSingleton<AppObject>);
}

AppObject::StaticData AppObject::s;
