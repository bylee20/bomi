#include "main.hpp"
#include "dialog/mbox.hpp"
#include "json/jrserver.hpp"
#include "player/jrplayer.hpp"
#include <QCryptographicHash>
#include <QElapsedTimer>

extern "C" void _exit(int);

DECLARE_LOG_CONTEXT(Main)

namespace OGL { auto check() -> QString; }

template<class F>
SIA measure(F func, int loop = 100000) -> quint64
{
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < loop; ++i)
        func(i);
    return timer.nsecsElapsed() / loop;
}

int main(int argc, char **argv) {
#ifdef BOMI_IMPORT_ICU
    Locale::importIcu();
    return 0;
#endif
#ifdef Q_OS_LINUX
    auto gtk_disable_setlocale
            = (void(*)(void))QLibrary::resolve(u"gtk-x11-2.0"_q,
                                               0, "gtk_disable_setlocale");
    if (gtk_disable_setlocale)
        gtk_disable_setlocale();
#endif
    QApplication::setAttribute(Qt::AA_X11InitThreads);
    registerType();

    QScopedPointer<App> app(new App(argc, argv));

#ifdef Q_OS_WIN
    const char sep = ';';
#else
    const char sep = ':';
#endif
    const auto envPath = qgetenv("PATH");
    qputenv("PATH", envPath + sep + QApplication::applicationDirPath().toLocal8Bit() + "/tools");

    for (auto fmt : QImageWriter::supportedImageFormats())
        writableImageExts.push_back(QString::fromLatin1(fmt));

    if (app->executeToQuit())
        return 0;

    const auto error = OGL::check();
    if (!error.isEmpty()) {
        MBox mbox(nullptr, MBox::Icon::Critical,
                  qApp->translate("OpenGL", "OpenGL Error"),
                  qApp->translate("OpenGL", "Error: %1\n\n"
                                            "Failed to check OpenGL support.\n"
                                            "It may help to update driver of "
                                            "graphics card.").arg(error));
        auto button = mbox.addButton(MBox::Button::Close);
        QObject::connect(button, &QPushButton::clicked, qApp, &QApplication::quit);
        mbox.mbox()->show();
        return app->exec();
    }
    qsrand(QDateTime::currentMSecsSinceEpoch());

    MainWindow *mw = new MainWindow;
    _Debug("Show MainWindow.");
    mw->show();
    app->setMainWindow(mw);
    _Debug("Start main event loop.");

    auto ret = app->exec();
    app->sendPostedEvents(nullptr, QEvent::DeferredDelete);
    app.reset();
    _Debug("Exit...");
#ifndef Q_OS_WIN
    std::quick_exit(ret);
#else
    std::exit(ret);
#endif
    return ret;
}
