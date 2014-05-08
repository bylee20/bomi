#ifndef APP_HPP
#define APP_HPP

#include "stdafx.hpp"

class QUrl;        class Mrl;
class MainWindow;    class QMenuBar;

class App : public QApplication {
    Q_OBJECT
public:
    App(int &argc, char **argv);
    ~App();
    auto setWindowTitle(QWidget *w, const QString &title) -> void;
    auto setMainWindow(MainWindow *mw) -> void;
    auto mainWindow() const -> MainWindow*;
    auto devices() const -> QStringList;
    auto styleName() const -> QString;
    auto isUnique() const -> bool;
    auto availableStyleNames() const -> QStringList;
#ifdef Q_OS_MAC
    QMenuBar *globalMenuBar() const;
#endif
    auto setStyleName(const QString &name) -> void;
    auto setLocale(const QLocale &locale) -> void;
    auto locale() const -> QLocale;
    auto setAlwaysOnTop(QWidget *widget, bool onTop) -> void;
    auto setScreensaverDisabled(bool disabled) -> void;
    auto setHeartbeat(const QString &command, int interval) -> void;
    auto setUnique(bool unique) -> void;
    auto shutdown() -> bool;
    auto runCommands() -> void;
    auto isOpenGLDebugLoggerRequested() const -> bool;
    auto setMprisActivated(bool activated) -> void;
    auto sendMessage(const QString &message, int timeout = 5000) -> bool;
    static auto defaultIcon() -> QIcon;
signals:
    void messageReceived(const QString &message);
private:
    static constexpr int ReopenEvent = QEvent::User + 1;
    auto event(QEvent *event) -> bool;
    struct Data;
    Data *d = nullptr;
};

#define cApp (*static_cast<App*>(qApp))

class LocalConnection : public QObject {
    Q_OBJECT
public:
    LocalConnection(const QString &id, QObject *parent = 0);
    ~LocalConnection();
    auto runServer() -> bool;
    auto sendMessage(const QString &message, int timeout) -> bool;
signals:
    void messageReceived(const QString &message);
private:
    struct Data;
    Data *d;
};

#endif // APPLICATION_HPP
