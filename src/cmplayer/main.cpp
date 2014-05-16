#include "app.hpp"
#include "translator.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "video/videoformat.hpp"
#include "video/hwacc.hpp"
#include "opengl/openglcompat.hpp"
#include "log.hpp"
#include "tmp.hpp"

DECLARE_LOG_CONTEXT(Main)

auto reg_top_level_item() -> void;
auto reg_button_box_item() -> void;
auto reg_busy_icon_item() -> void;
auto reg_downloader() -> void;
auto reg_history_model() -> void;
auto reg_playlist_model() -> void;
auto reg_app_object() -> void;
auto reg_settings_object() -> void;
auto reg_theme_object() -> void;

int main(int argc, char **argv) {
    qputenv("PX_MODULE_PATH", "/this-is-dummy-path-to-disable-libproxy");
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale = (void(*)(void))QLibrary::resolve(_L("gtk-x11-2.0"), 0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
    QApplication::setAttribute(Qt::AA_X11InitThreads);

    reg_theme_object();
    reg_downloader();
    reg_history_model();
    reg_playlist_model();
    reg_top_level_item();
    reg_button_box_item();
    reg_busy_icon_item();
    reg_app_object();
    reg_settings_object();
    PlayEngine::registerObjects();
    App app(argc, argv);
    if (app.isUnique() && app.sendMessage(app.arguments().join("[:sep:]"))) {
        _Info("Another instance of CMPlayer is already running. Exit this...");
        return 0;
    }

    OpenGLCompat::check();
    HwAcc::initialize();
    MainWindow *mw = new MainWindow;
    _Debug("Show MainWindow.");
    mw->show();
    app.setMainWindow(mw);
    _Debug("Start main event loop.");
    auto ret = app.exec();
    HwAcc::finalize();
    _Debug("Exit...");
    return ret;
}
