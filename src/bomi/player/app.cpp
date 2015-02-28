#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "misc/localconnection.hpp"
#include "misc/logoption.hpp"
#include "misc/json.hpp"
#include "misc/locale.hpp"
#include "misc/objectstorage.hpp"
#include "os/os.hpp"
#include <clocale>
#include <QStyleFactory>
#include <QMenuBar>
#include <QFileOpenEvent>
#include <QJsonDocument>
#include <QCommandLineParser>
#include <QFontDatabase>

#ifdef Q_OS_LINUX
#include "player/mpris.hpp"
#endif

#ifdef main
#undef main
#endif

DECLARE_LOG_CONTEXT(App)

namespace Global {
auto open_folders() -> QMap<QString, QString>;
auto set_open_folders(const QMap<QString, QString> &folders) -> void;
auto _SetWindowTitle(QWidget *w, const QString &title) -> void
{ cApp.setWindowTitle(w, title); }
}

auto root_menu_execute(const QString &longId, const QString &argument) -> bool;
auto translator_load(const Locale &locale) -> bool;

enum class LineCmd {
    Wake, Open, Action, LogLevel, OpenGLDebug, Debug
};

struct App::Data {
    Data(App *p): p(p) {}
    App *p = nullptr;
    bool gldebug = false;
    QStringList styleNames;
    QString styleName = qApp->style()->objectName();
    bool unique = true;
    QMap<QString, QVariant> openFolders;
    Mrl pended;
#ifdef Q_OS_MAC
    QMenuBar *mb = new QMenuBar;
#else
#if defined(Q_OS_LINUX)
    mpris::RootObject *mpris = nullptr;
#endif
    QMenuBar *mb = nullptr;
#endif
    MainWindow *main = nullptr;
    ObjectStorage storage;

    LogOption logOption = LogOption::default_();
    Locale locale;
    QCommandLineOption dummy{u"__dummy__"_q};
    QCommandLineParser cmdParser, msgParser;
    QMap<LineCmd, QCommandLineOption> options;
    LocalConnection connection = {u"net.xylosper.bomi"_q, nullptr};
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

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
    auto execute(const QCommandLineParser *parser) -> Log::Level
    {
        auto isSet = [parser, this] (LineCmd cmd)
            { return parser->isSet(options.value(cmd, dummy)); };
        auto value = [parser, this] (LineCmd cmd)
            { return parser->value(options.value(cmd, dummy)); };
        auto values = [parser, this] (LineCmd cmd)
            { return parser->values(options.value(cmd, dummy)); };
        Log::Level lvStdOut = Log::Off;
        if (isSet(LineCmd::LogLevel))
            lvStdOut = Log::level(value(LineCmd::LogLevel));
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
            lvStdOut = qMax(lvStdOut, Log::Debug);
            gldebug = true;
        }
        return lvStdOut;
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

    auto import() -> bool
    {
        auto copy = [] (const QString &from, const QString &to) -> bool
        {
            if (QFile::exists(to) || !QFile::exists(from))
                return false;
            if (QFile::copy(from, to)) {
                qDebug("Import old file: %s", from.toLocal8Bit().constData());
                return true;
            } else {
                qDebug("Failed to import old file: %s", from.toLocal8Bit().constData());
                return false;
            }
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
        return copy(path(Location::Config, ".conf"_a), _WritablePath(Location::Config) % ".conf"_a);
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
    OS::initialize();

    d->addOption(LineCmd::Open, u"open"_q,
                 tr("Open given %1 for file path or URL."), u"mrl"_q);
    d->addOption(LineCmd::Wake, u"wake"_q,
                 tr("Bring the application window in front."));
    d->addOption(LineCmd::Action, u"action"_q,
                 tr("Exectute %1 action or open %1 menu."), u"id"_q);
    d->addOption(LineCmd::LogLevel, u"log-level"_q,
                 tr("Maximum verbosity for log. %1 should be one of nexts:")
                 % "\n    "_a % Log::levelNames().join(u", "_q), u"lv"_q);
    d->addOption(LineCmd::OpenGLDebug, u"opengl-debug"_q,
                 tr("Turn on OpenGL debug logger."));
    d->addOption(LineCmd::Debug, u"debug"_q,
                 tr("Turn on options for debugging."));
    d->getCommandParser(&d->cmdParser)->process(arguments());
    d->getCommandParser(&d->msgParser);
    auto lvStdOut = d->execute(&d->cmdParser);

    d->import();

    d->storage.setObject(this, u"application"_q);
    d->storage.add("locale", &d->locale);
    d->storage.json("log-option", &d->logOption);
    d->storage.add("style-name", &d->styleName);
    d->storage.add("unique", &d->unique);
    d->storage.add("open-folders", open_folders, set_open_folders);
    d->storage.add("font");
    d->storage.add("fixedFont");
    d->storage.restore();

    setLocale(d->locale);

    auto logOption = d->logOption;
    if (logOption.level(LogOutput::StdOut) < lvStdOut)
        logOption.setLevel(LogOutput::StdOut, lvStdOut);
    Log::setOption(logOption);

    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(defaultIcon());
#endif

    d->styleNames = [this] () {
        auto names = QStyleFactory::keys();
        const auto defaultName = style()->objectName();
        for (auto it = ++names.begin(); it != names.end(); ++it) {
            if (defaultName.compare(*it, Qt::CaseInsensitive) == 0) {
                const auto name = *it;
                names.erase(it);
                names.prepend(name);
                break;
            }
        }
        return names;
    }();
    auto makeStyle = [&]() {
        auto name = d->styleName;
        if (style()->objectName().compare(name, Qt::CaseInsensitive) == 0)
            return;
        if (!d->styleNames.contains(name, Qt::CaseInsensitive))
            return;
        setStyle(QStyleFactory::create(name));
    };
    makeStyle();
    connect(&d->connection, &LocalConnection::messageReceived,
            this, &App::handleMessage);
}

App::~App() {
    setMprisActivated(false);
    delete d->main;
    delete d->mb;
    delete d;
    OS::finalize();
}

auto App::save() const -> void
{
    d->storage.save();
}

auto App::load() -> void
{
    d->storage.restore();
}

auto App::handleMessage(const QByteArray &message) -> void
{
    QJsonParseError error;
    const auto msg = QJsonDocument::fromJson(message, &error).object();
    Q_ASSERT(!error.error);

    const auto type = msg[u"type"_q].toInt();
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

auto App::sendMessage(MessageType type, const QJsonValue &json, int timeout) -> bool
{
    QJsonObject message;
    message[u"type"_q] = (int)type;
    message[u"contents"_q] = json;
    return d->connection.sendMessage(QJsonDocument(message).toJson(), timeout);
}

auto App::setLocale(const Locale &locale) -> void
{
    if (translator_load(locale))
        d->locale = locale;
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
    if (!d->styleNames.contains(name, Qt::CaseInsensitive))
        return;
    if (style()->objectName().compare(name, Qt::CaseInsensitive) == 0)
        return;
    setStyle(QStyleFactory::create(name));
    d->styleName = style()->objectName();
}

auto App::setUnique(bool unique) -> void
{
    d->unique = unique;
}

auto App::styleName() const -> QString
{
    return style()->objectName();
}

auto App::isUnique() const -> bool
{
    return d->unique;
}

auto App::logOption() const -> LogOption
{
    return d->logOption;
}

auto App::setLogOption(const LogOption &option) -> void
{
    d->logOption = option;
}

auto App::setFixedFont(const QFont &font) -> void
{
    d->fixedFont = font;
}

auto App::fixedFont() const -> QFont
{
    return d->fixedFont;
}
