#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "misc/localconnection.hpp"
#include "misc/log.hpp"
#include "misc/json.hpp"
#include "misc/locale.hpp"

#if defined(Q_OS_MAC)
#include "app_mac.hpp"
#elif defined(Q_OS_LINUX)
#include "app_x11.hpp"
#include "mpris.hpp"
#endif

DECLARE_LOG_CONTEXT(App)

namespace Pch {
auto open_folders() -> QMap<QString, QString>;
auto set_open_folders(const QMap<QString, QString> &folders) -> void;
}

auto root_menu_execute(const QString &longId, const QString &argument) -> bool;
auto translator_load(const Locale &locale) -> bool;

auto set_window_title(QWidget *w, const QString &title) -> void
{
    cApp.setWindowTitle(w, title);
}

enum class LineCmd {
    Wake, Open, Action, LogLevel, OpenGLDebug, Debug
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

    Locale locale;
    QCommandLineOption dummy{u"__dummy__"_q};
    QCommandLineParser cmdParser, msgParser;
    QMap<LineCmd, QCommandLineOption> options;
    LocalConnection connection = {u"net.xylosper.bomi"_q, nullptr};
    auto open(const Mrl &mrl) -> void
    {
        if (!main || !main->isSceneGraphInitialized())
            pended = mrl;
        else
            main->openFromFileManager(mrl);
    }
    auto addOption(LineCmd cmd, const QString &name, const QString &desc,
                   const QString &valName = QString(),
                   const QString &def = QString()) -> void
    { addOption(cmd, QStringList(name), desc, valName, def); }
    auto addOption(LineCmd cmd, const QStringList &names, const QString &desc,
                   const QString &valName = QString(),
                   const QString &def = QString()) -> void
    {
        if (desc.contains("%1"_a)) {
            const QCommandLineOption opt(names, desc.arg('<'_q % valName % '>'_q),
                                         valName, def);
            options.insert(cmd, opt);
        } else
            options.insert(cmd, QCommandLineOption(names, desc, valName, def));
    }
    auto execute(const QCommandLineParser *parser) -> void
    {
        auto isSet = [parser, this] (LineCmd cmd)
            { return parser->isSet(options.value(cmd, dummy)); };
        auto value = [parser, this] (LineCmd cmd)
            { return parser->value(options.value(cmd, dummy)); };
        auto values = [parser, this] (LineCmd cmd)
            { return parser->values(options.value(cmd, dummy)); };
        if (isSet(LineCmd::LogLevel))
            Log::setMaximumLevel(value(LineCmd::LogLevel));
        if (isSet(LineCmd::OpenGLDebug))
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
        }
    }
    auto getCommandParser(QCommandLineParser *parser) const
    -> QCommandLineParser*
    {
        for (auto it = options.begin(); it != options.end(); ++it)
            parser->addOption(*it);
        parser->addHelpOption();
        parser->addVersionOption();
        const auto desc = tr("The file path or URL to open.");
        parser->addPositionalArgument(u"mrl"_q, desc, u"mrl"_q);
        return parser;
    }

    const QString APP_GROUP{u"application"_q};
    template<class T>
    auto read(const char *key, const T &t = T()) -> T
    {
        QSettings s;
        s.beginGroup(APP_GROUP);
        auto ret = s.value(_L(key), t).template value<T>();
        s.endGroup();
        return ret;
    }
    template<class T>
    auto write(const char *key, const T &t) -> void
    {
        QSettings s;
        s.beginGroup(APP_GROUP);
        s.setValue(_L(key), t);
        s.endGroup();
    }
    auto import() -> void
    {
        auto copy = [] (const QString &from, const QString &to) -> void
        {
            if (QFile::exists(to) || !QFile::exists(from))
                return;
            if (QFile::copy(from, to))
                _Info("Import old file: %%", from);
            else
                _Error("Failed to import old file: %%", from);
        };

        auto path = [] (Location loc, const QString &fileName) -> QString
        {
            const auto std = static_cast<QStandardPaths::StandardLocation>(loc);
            auto path = QStandardPaths::writableLocation(std);
            if (loc == Location::Config)
                path += '/'_q % qApp->organizationName()
                      % '/'_q % "CMPlayer"_a;
            else {
                const QString target = "/bomi"_a;
                path.replace(path.lastIndexOf(target), target.size(), "/CMPlayer"_a);
            }
            return path % fileName;
        };

        copy(path(Location::Data, "/appstate.json"_a), _WritablePath(Location::Config) % "/appstate.json"_a);
        copy(path(Location::Data, "/history.db"_a), _WritablePath(Location::Config) % "/history.db"_a);
        copy(path(Location::Config, "/pref.json"_a), _WritablePath(Location::Config) % "/pref.json"_a);
        copy(path(Location::Config, ".conf"_a), _WritablePath(Location::Config) % ".conf"_a);
    }
};

App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data(this)) {
#ifdef Q_OS_LINUX
    setlocale(LC_NUMERIC,"C");
#endif
    setOrganizationName(u"xylosper"_q);
    setOrganizationDomain(u"xylosper.net"_q);
    setApplicationName(_L(name()));
    setApplicationVersion(_L(version()));
    d->import();

    setLocale(Locale::fromVariant(d->read("locale", Locale().toVariant())));

    d->addOption(LineCmd::Open, u"open"_q,
                 tr("Open given %1 for file path or URL."), u"mrl"_q);
    d->addOption(LineCmd::Wake, u"wake"_q,
                 tr("Bring the application window in front."));
    d->addOption(LineCmd::Action, u"action"_q,
                 tr("Exectute %1 action or open %1 menu."), u"id"_q);
    d->addOption(LineCmd::LogLevel, u"log-level"_q,
                 tr("Maximum verbosity for log. %1 should be one of nexts:")
                 % "\n    "_a % Log::options().join(u", "_q), u"lv"_q);
    d->addOption(LineCmd::OpenGLDebug, u"opengl-debug"_q,
                 tr("Turn on OpenGL debug logger."));
    d->addOption(LineCmd::Debug, u"debug"_q,
                 tr("Turn on options for debugging."));
    d->getCommandParser(&d->cmdParser)->process(arguments());
    d->getCommandParser(&d->msgParser);
    d->execute(&d->cmdParser);

    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(defaultIcon());
#endif

    auto makeStyle = [&]() {
        auto name = d->read("style", styleName());
        if (style()->objectName().compare(name, QCI) == 0)
            return;
        if (!d->styleNames.contains(name, QCI))
            return;
        setStyle(QStyleFactory::create(name));
    };
    d->styleNames = [this] () {
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
    }();
    makeStyle();
    connect(&d->connection, &LocalConnection::messageReceived,
             this, &App::handleMessage);
    const auto map = d->read<QMap<QString, QVariant>>("open_folders");
    QMap<QString, QString> folders;
    for (auto it = map.begin(); it != map.end(); ++it)
        folders.insert(it.key(), it->toString());
    set_open_folders(folders);
}

App::~App() {
    setMprisActivated(false);
    const auto folders = Pch::open_folders();
    QMap<QString, QVariant> map;
    for (auto it = folders.begin(); it != folders.end(); ++it)
        map.insert(it.key(), *it);
    d->write("open_folders", map);
    delete d->main;
    delete d->mb;
    delete d;
}

auto App::handleMessage(const QByteArray &message) -> void
{
    QJsonParseError error;
    const auto msg = QJsonDocument::fromJson(message, &error).object();
    Q_ASSERT(!error.error);

    const auto type = _JsonToInt(msg[u"type"_q]);
    const auto contents = msg[u"contents"_q];
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
    _Trace("Set window title of %% to '%%'.",
           widget->metaObject()->className(), title);
    const QString text = title % (title.isEmpty() ? u""_q : u" - "_q)
                         % displayName();
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
        icon.addFile(u":/img/bomi16.png"_q, QSize(16, 16));
        icon.addFile(u":/img/bomi22.png"_q, QSize(22, 22));
        icon.addFile(u":/img/bomi24.png"_q, QSize(24, 24));
        icon.addFile(u":/img/bomi32.png"_q, QSize(32, 32));
        icon.addFile(u":/img/bomi48.png"_q, QSize(48, 48));
        icon.addFile(u":/img/bomi64.png"_q, QSize(64, 64));
        icon.addFile(u":/img/bomi128.png"_q, QSize(128, 128));
        icon.addFile(u":/img/bomi256.png"_q, QSize(256, 256));
        icon.addFile(u":/img/bomi-logo.png"_q, QSize(512, 512));
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

auto App::setLocale(const Locale &locale) -> void
{
    if (translator_load(locale)) {
        d->locale = locale;
        d->write("locale", d->locale.toVariant());
    }
}

auto App::locale() const -> Locale
{
    return d->locale;
}

auto App::defaultStyleName() const -> QString
{
    return d->styleNames.front();
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
    d->write("style", name);
}

auto App::setUnique(bool unique) -> void
{
    d->write("unique", unique);
}

auto App::styleName() const -> QString
{
    return style()->objectName();
}

auto App::isUnique() const -> bool
{
    return d->read("unique", true);
}

auto App::shutdown() -> bool
{
    return d->helper.shutdown();
}
