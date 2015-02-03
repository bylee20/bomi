#include "appobject.hpp"
#include "windowobject.hpp"
#include "player/rootmenu.hpp"

auto reg_app_object() -> void {
    qmlRegisterSingletonType<AppObject>("bomi", 1, 0, "App",
                                        _QmlSingleton<AppObject>);
    qmlRegisterType<WindowObject>();
}

AppObject::StaticData AppObject::s;

auto AppObject::setWindow(MainWindow *window) -> void
{
    static WindowObject o;
    o.set(window);
    s.window = &o;
}

auto AppObject::description(const QString &actionId) -> QString
{
    return RootMenu::instance().description(actionId);
}
