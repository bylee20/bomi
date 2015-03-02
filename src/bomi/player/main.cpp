#include "main.hpp"
#include "json/jrserver.hpp"
#include "player/jrplayer.hpp"

DECLARE_LOG_CONTEXT(Main)

namespace OGL { auto check() -> void; }

int main(int argc, char **argv) {
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale
            = (void(*)(void))QLibrary::resolve(u"gtk-x11-2.0"_q,
                                               0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
    QApplication::setAttribute(Qt::AA_X11InitThreads);
    registerType();

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
