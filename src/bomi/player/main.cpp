#include "app.hpp"
#include "stdafx.hpp"
#include "mainwindow.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "quick/circularimageitem.hpp"
#include "quick/maskareaitem.hpp"
#include <QImageWriter>

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
auto reg_play_engine() -> void;

namespace Global {
extern QStringList writableImageExts;
}

namespace OGL { auto check() -> void; }

int main(int argc, char **argv) {
    qputenv("PX_MODULE_PATH", "/this-is-dummy-path-to-disable-libproxy");
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale
            = (void(*)(void))QLibrary::resolve(u"gtk-x11-2.0"_q,
                                               0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
    QApplication::setAttribute(Qt::AA_X11InitThreads);

    qmlRegisterType<CircularImageItem>("bomi", 1, 0, "CircularImage");
    qmlRegisterType<MaskAreaItem>("bomi", 1, 0, "MaskArea");
    qmlRegisterType<MouseEventObject>();
    reg_theme_object();
    reg_downloader();
    reg_history_model();
    reg_playlist_model();
    reg_top_level_item();
    reg_button_box_item();
    reg_busy_icon_item();
    reg_app_object();
    reg_settings_object();
    reg_play_engine();

    App app(argc, argv);
    for (auto fmt : QImageWriter::supportedImageFormats())
        writableImageExts.push_back(QString::fromLatin1(fmt));

    if (app.isUnique()
            && app.sendMessage(App::CommandLine, _ToJson(app.arguments()))) {
        _Info("Another instance of bomi is already running. Exit this...");
        return 0;
    }

    OGL::check();
    MainWindow *mw = new MainWindow;
    _Debug("Show MainWindow.");
    mw->show();
    app.setMainWindow(mw);
    _Debug("Start main event loop.");
    auto ret = app.exec();
    _Debug("Exit...");
    return ret;
}
