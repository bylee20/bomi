#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "misc/localconnection.hpp"
#include "misc/log.hpp"
#include "misc/record.hpp"
#include "misc/json.hpp"

#if defined(Q_OS_MAC)
#include "app_mac.hpp"
#elif defined(Q_OS_LINUX)
#include "app_x11.hpp"
#include "mpris.hpp"
#endif

#define APP_GROUP "application"_a

DECLARE_LOG_CONTEXT(App)

namespace Pch {
auto open_folders() -> QMap<QString, QString>;
auto set_open_folders(const QMap<QString, QString> &folders) -> void;
}

auto root_menu_execute(const QString &longId, const QString &argument) -> bool;
auto translator_load(const QLocale &locale) -> bool;

auto set_window_title(QWidget *w, const QString &title) -> void
{
    cApp.setWindowTitle(w, title);
}

enum class LineCmd {
    Wake,
    Open,
    Action,
    LogLevel,
    OpenGLDebug,
    Debug
};

struct App::Data {
    Data(App *p): p(p) {}
    App *p = nullptr;
    bool gldebug = false;
    QStringList styleNames;
    Mrl pended;
#ifdef Q_OS_MAC
    QMenuBar *mb = new QMenuBar;
    AppMac helper;
#else
#if defined(Q_OS_LINUX)
    AppX11 helper;
    mpris::RootObject *mpris = nullptr;
#endif
    QMenuBar *mb = nullptr;
#endif
    MainWindow *main = nullptr;

    QLocale locale = QLocale::system();
    QCommandLineOption dummy{"__dummy__"};
    QCommandLineParser cmdParser, msgParser;
    QMap<LineCmd, QCommandLineOption> options;
    LocalConnection connection = {"net.xylosper.CMPlayer", nullptr};
    auto open(const Mrl &mrl) -> void
    {
        if (!main || !main->isSceneGraphInitialized())
            pended = mrl;
        else
            main->openFromFileManager(mrl);
    }
    void addOption(LineCmd cmd, const char *name, const QString desc, const char *valueName = "", const QString &def = QString()) {
        addOption(cmd, QStringList() << _L(name), desc, valueName, def);
    }
    void addOption(LineCmd cmd, const QStringList &names, const QString desc, const char *valueName = "", const QString &def = QString()) {
        const QLatin1String vname = _L(valueName);
        if (desc.contains("%1"_a))
            options.insert(cmd, QCommandLineOption(names, desc.arg('<' % vname % '>'), vname, def));
        else
            options.insert(cmd, QCommandLineOption(names, desc, vname, def));
    }
    void execute(const QCommandLineParser *parser) {
        auto isSet = [parser, this] (LineCmd cmd) { return parser->isSet(options.value(cmd, dummy)); };
        auto value = [parser, this] (LineCmd cmd) { return parser->value(options.value(cmd, dummy)); };
        auto values = [parser, this] (LineCmd cmd) { return parser->values(options.value(cmd, dummy)); };
        if (isSet(LineCmd::LogLevel))
            Log::setMaximumLevel(value(LineCmd::LogLevel));
        if (isSet(LineCmd::OpenGLDebug) || qgetenv("CMPLAYER_GL_DEBUG").toInt())
            gldebug = true;
        if (main) {
            if (isSet(LineCmd::Wake))
                main->wake();
            Mrl mrl;
            if (isSet(LineCmd::Open))
                mrl = Mrl(value(LineCmd::Open));
            if (!parser->positionalArguments().isEmpty())
                mrl = Mrl(parser->positionalArguments().first());
            if (!mrl.isEmpty())
                open(mrl);
            const auto args = values(LineCmd::Action);
            if (!args.isEmpty())
                root_menu_execute(args[0], args.value(1));
        }
        if (isSet(LineCmd::Debug)) {
            if (Log::maximumLevel() < Log::Debug)
                Log::setMaximumLevel(Log::Debug);
            gldebug = true;
            qputenv("CMPLAYER_MPV_VERBOSE", "v");
        }
    }
    QCommandLineParser *getCommandParser(QCommandLineParser *parser) const {
        for (auto it = options.begin(); it != options.end(); ++it)
            parser->addOption(*it);
        parser->addHelpOption();
        parser->addVersionOption();
        parser->addPositionalArgument("mrl", tr("The file path or URL to open."), "mrl");
        return parser;
    }
};

App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data(this)) {
    Record r(APP_GROUP);

#ifdef Q_OS_LINUX
    setlocale(LC_NUMERIC,"C");
#endif
    setOrganizationName("xylosper");
    setOrganizationDomain("xylosper.net");
    setApplicationName(name());
    setApplicationVersion(version());
    setLocale(r.value("locale", QLocale::system()).toLocale());

    d->addOption(LineCmd::Open, "open", tr("Open given %1 for file path or URL."), "mrl");
    d->addOption(LineCmd::Wake, QStringList() << "wake", tr("Bring the application window in front."));
    d->addOption(LineCmd::Action, "action", tr("Exectute %1 action or open %1 menu."), "id");
    d->addOption(LineCmd::LogLevel, "log-level", tr("Maximum verbosity for log. %1 should be one of nexts:")
                 % "\n    " % Log::options().join(", "), "lv");
    d->addOption(LineCmd::OpenGLDebug, "opengl-debug", tr("Turn on OpenGL debug logger."));
    d->addOption(LineCmd::Debug, "debug", tr("Turn on options for debugging."));
    d->getCommandParser(&d->cmdParser)->process(arguments());
    d->getCommandParser(&d->msgParser);
    d->execute(&d->cmdParser);

#if defined(Q_OS_MAC) && defined(CMPLAYER_RELEASE)
    static const QByteArray path = QApplication::applicationDirPath().toLocal8Bit();
    _Debug("Set $LIBQUVI_SCRIPTSDIR=\"%%\"", QApplication::applicationDirPath());
    if (setenv("LIBQUVI_SCRIPTSDIR", path.data(), 1) < 0)
        _Error("Cannot set $LIBQUVI_SCRIPTSDIR. Some streaming functions won't work.");
#endif

    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(defaultIcon());
#endif
    auto makeStyleNameList = [this] () {
        auto names = QStyleFactory::keys();
        const auto defaultName = style()->objectName();
        for (auto it = ++names.begin(); it != names.end(); ++it) {
            if (defaultName.compare(*it, QCI) == 0) {
                const auto name = *it;
                names.erase(it);
                names.prepend(name);
                break;
            }
        }
        return names;
    };
    auto makeStyle = [&]() {
        auto name = r.value("style", styleName()).toString();
        if (style()->objectName().compare(name, QCI) == 0)
            return;
        if (!d->styleNames.contains(name, QCI))
            return;
        setStyle(QStyleFactory::create(name));
    };
    d->styleNames = makeStyleNameList();
    makeStyle();
    connect(&d->connection, &LocalConnection::messageReceived,
             this, &App::handleMessage);
    const auto map = r.value("open_folders").toMap();
    QMap<QString, QString> folders;
    for (auto it = map.begin(); it != map.end(); ++it)
        folders.insert(it.key(), it->toString());
    set_open_folders(folders);
}

App::~App() {
    setMprisActivated(false);
    Record r(APP_GROUP);
    const auto folders = Pch::open_folders();
    QMap<QString, QVariant> map;
    for (auto it = folders.begin(); it != folders.end(); ++it)
        map.insert(it.key(), *it);
    r.setValue("open_folders", map);
    delete d->main;
    delete d->mb;
    delete d;
}

auto App::handleMessage(const QByteArray &message) -> void
{
    QJsonParseError error;
    const auto msg = QJsonDocument::fromJson(message, &error).object();
    Q_ASSERT(!error.error);

    const auto type = _JsonToInt(msg["type"]);
    const auto contents = msg["contents"];
    switch (type) {
    case CommandLine:
        d->msgParser.parse(_FromJson<QStringList>(contents));
        d->execute(&d->msgParser);
        break;
    default:
        _Error("Unknown message: %%", message);
        break;
    }
}

auto App::isOpenGLDebugLoggerRequested() const -> bool
{
    return d->gldebug;
}

auto App::setMainWindow(MainWindow *mw) -> void
{
    d->main = mw;
#ifndef Q_OS_MAC
    d->main->setWindowIcon(defaultIcon());
#endif
    connect(d->main, &MainWindow::sceneGraphInitialized, this, [this] () {
        if (!d->pended.isEmpty()) {
            d->main->openFromFileManager(d->pended);
            d->pended = Mrl();
        }
    }, Qt::QueuedConnection);
}

auto App::setWindowTitle(QWidget *widget, const QString &title) -> void
{
//    _Trace("Set window title of %% to '%%'.", widget->metaObject()->className(), title);
    const QString text = title % (title.isEmpty() ? "" : " - ") % applicationName();
    widget->setWindowTitle(text);
#ifdef Q_OS_LINUX
    d->helper.setWmName(widget, text);
#endif
}

auto App::setMprisActivated(bool activated) -> void
{
#ifdef Q_OS_LINUX
    if (activated && !d->mpris)
        d->mpris = new mpris::RootObject(this);
    else if (!activated && d->mpris)
        _Delete(d->mpris);
#else
    Q_UNUSED(activated);
#endif
}

auto App::mainWindow() const -> MainWindow*
{
    return d->main;
}

auto App::defaultIcon() -> QIcon
{
    static QIcon icon;
    static bool init = false;
    if (!init) {
        icon.addFile(":/img/cmplayer16.png", QSize(16, 16));
        icon.addFile(":/img/cmplayer22.png", QSize(22, 22));
        icon.addFile(":/img/cmplayer24.png", QSize(24, 24));
        icon.addFile(":/img/cmplayer32.png", QSize(32, 32));
        icon.addFile(":/img/cmplayer48.png", QSize(48, 48));
        icon.addFile(":/img/cmplayer64.png", QSize(64, 64));
        icon.addFile(":/img/cmplayer128.png", QSize(128, 128));
        icon.addFile(":/img/cmplayer256.png", QSize(256, 256));
        icon.addFile(":/img/cmplayer-logo.png", QSize(512, 512));
        init = true;
    }
    return icon;
}

auto App::setAlwaysOnTop(QWidget *widget, bool onTop) -> void
{
    d->helper.setAlwaysOnTop(widget, onTop);
}

auto App::setHeartbeat(const QString &command, int interval) -> void
{
    Q_UNUSED(command); Q_UNUSED(interval);
#ifdef Q_OS_LINUX
    d->helper.setHeartbeat(command, interval);
#endif
}

auto App::setScreensaverDisabled(bool disabled) -> void
{
    d->helper.setScreensaverDisabled(disabled);
}

auto App::event(QEvent *event) -> bool
{
    switch ((int)event->type()) {
    case QEvent::FileOpen: {
        const auto ev = static_cast<QFileOpenEvent*>(event);
        d->open(Mrl(ev->url().toString()));
        event->accept();
        return true;
    } case ReopenEvent:
        d->main->show();
        event->accept();
        return true;
    default:
        return QApplication::event(event);
    }
}

auto App::devices() const -> QStringList
{
    return d->helper.devices();
}

#ifdef Q_OS_MAC
auto App::globalMenuBar() const -> QMenuBar*
{
    return d->mb;
}
#endif

auto App::runCommands() -> void
{
    d->execute(&d->cmdParser);
}

auto App::sendMessage(const QByteArray &message, int timeout) -> bool
{
    return d->connection.sendMessage(message, timeout);
}

auto App::setLocale(const QLocale &locale) -> void
{
    if (translator_load(locale)) {
        Record r(APP_GROUP);
        r.write(d->locale = locale, "locale");
    }
}

auto App::locale() const -> QLocale
{
    return d->locale;
}

auto App::availableStyleNames() const -> QStringList
{
    return d->styleNames;
}

auto App::setStyleName(const QString &name) -> void
{
    if (!d->styleNames.contains(name, QCI))
        return;
    if (style()->objectName().compare(name, QCI) == 0)
        return;
    setStyle(QStyleFactory::create(name));
    Record r(APP_GROUP);
    r.write(name, "style");
}

auto App::setUnique(bool unique) -> void
{
    Record r(APP_GROUP);
    r.write(unique, "unique");
}

auto App::styleName() const -> QString
{
    return style()->objectName();
}

auto App::isUnique() const -> bool
{
    Record r(APP_GROUP);
    return r.value("unique", true).toBool();
}

#undef APP_GROUP

auto App::shutdown() -> bool
{
    return d->helper.shutdown();
}
