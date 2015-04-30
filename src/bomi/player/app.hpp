#ifndef APP_HPP
#define APP_HPP

class QUrl;                             class Mrl;
class MainWindow;                       class QMenuBar;
class Locale;                           struct LogOption;

class App : public QApplication {
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(QFont fixedFont READ fixedFont WRITE setFixedFont)
public:
    enum MessageType {
        CommandLine = 1
    };
    App(int &argc, char **argv);
    ~App();
    auto setWindowTitle(QWidget *w, const QString &title) -> void;
    auto setWindowTitle(QWindow *w, const QString &title) -> void;
    auto setMainWindow(MainWindow *mw) -> void;
    auto mainWindow() const -> MainWindow*;
    auto styleName() const -> QString;
    auto isUnique() const -> bool;
    auto executeToQuit() -> bool;
    auto availableStyleNames() const -> QStringList;
    auto setUseLocalConfig(bool local) -> void;
    auto useLocalConfig() const -> bool;
#ifdef Q_OS_MAC
    auto globalMenuBar() const -> QMenuBar*;
#endif
    auto setStyleName(const QString &name) -> void;
    auto setLocale(const Locale &locale) -> void;
    auto defaultStyleName() const -> QString;
    auto locale() const -> Locale;
    auto setLogOption(const LogOption &option) -> void;
    auto logOption() const -> LogOption;
    auto setUnique(bool unique) -> void;
    auto runCommands() -> void;
    auto isOpenGLDebugLoggerRequested() const -> bool;
    auto setMprisActivated(bool activated) -> void;
    auto sendMessage(MessageType type, const QJsonValue &t, int timeout = 5000) -> bool;
    auto sendMessage(MessageType type, const QStringList &t, int timeout = 5000) -> bool;
    auto save() const -> void;
    auto load() -> void;
    auto setFixedFont(const QFont &font) -> void;
    auto fixedFont() const -> QFont;
    static auto version() -> const char*;
    static constexpr auto name() -> const char* { return "bomi"; }
    static auto displayName() -> QString { return tr("bomi"); }
    static auto defaultIcon() -> QIcon;
private:
    auto handleMessage(const QByteArray &message) -> void;
    static constexpr int ReopenEvent = QEvent::User + 1;
    auto event(QEvent *event) -> bool;
    struct Data;
    Data *d = nullptr;
};

#define cApp (*static_cast<App*>(qApp))

#endif // APPLICATION_HPP
