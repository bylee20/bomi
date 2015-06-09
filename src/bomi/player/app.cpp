#include "app.hpp"
#include "mrl.hpp"
#include "mainwindow.hpp"
#include "misc/localconnection.hpp"
#include "misc/logoption.hpp"
#include "misc/json.hpp"
#include "misc/locale.hpp"
#include "misc/objectstorage.hpp"
#include "quick/appobject.hpp"
#include "rootmenu.hpp"
#include "os/os.hpp"
#include <clocale>
#include <QStyleFactory>
#include <QMenuBar>
#include <QFileOpenEvent>
#include <QJsonDocument>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QSettings>

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
auto _OldConfigPath() -> QString;
auto _SetWindowTitle(QWidget *w, const QString &title) -> void
{ cApp.setWindowTitle(w, title); }
extern bool useLocalConfig;
}

auto translator_load(const Locale &locale) -> bool;

enum class LineCmd {
    Wake, Open, Action, LogLevel, Debug,
    DumpApiTree, DumpActionList, WinAssoc, WinUnassoc, WinAssocDefault,
    SetSubtitle, AddSubtitle,
};

static const QCommandLineOption s_dummy{u"__dummy__"_q};

class CommandParser {
public:
    CommandParser();
    auto addOption(LineCmd cmd, const QString &name, const QString &desc,
                   const QString &valName = QString(),
                   const QString &def = QString()) -> void
        { addOption(cmd, QStringList(name), desc, valName, def); }
    auto option(LineCmd cmd) const -> QCommandLineOption { return m_options.value(cmd, s_dummy); }
    auto isSet(LineCmd cmd) const -> bool { return m_parser.isSet(option(cmd)); }
    auto value(LineCmd cmd) const -> QString  { return m_parser.value(option(cmd)); }
    auto values(LineCmd cmd) const -> QStringList { return m_parser.values(option(cmd)); }
    auto parse(const QStringList &args) -> void { m_parser.process(args); }
    auto name(LineCmd cmd) const -> QString { return option(cmd).names().first(); }
    auto toJson() const -> QJsonArray
    {
        QStringList args;
        args.push_back(qApp->applicationFilePath());
        auto put = [&] (LineCmd cmd) {
            if (!isSet(cmd)) return false;
            args.push_back("--"_a % name(cmd)); return true;
        };
        put(LineCmd::Wake);
        if (put(LineCmd::SetSubtitle))
            args.push_back(QFileInfo(value(LineCmd::SetSubtitle)).absoluteFilePath());
        if (put(LineCmd::AddSubtitle))
            args.push_back(QFileInfo(value(LineCmd::AddSubtitle)).absoluteFilePath());
        if (put(LineCmd::Action))
            args.push_back(value(LineCmd::Action));
        const auto mrl = this->mrl();
        if (!mrl.isEmpty())
            args.push_back(mrl.toString());
        return _ToJson(args);
    }
    auto stdoutLogLevel() const -> Log::Level
    {
        Log::Level lv = Log::Off;
        if (isSet(LineCmd::LogLevel))
            lv = Log::level(value(LineCmd::LogLevel));
        if (isSet(LineCmd::Debug))
            lv = qMax(lv, Log::Debug);
        return lv;
    }
    auto mrl() const -> Mrl
    {
        if (isSet(LineCmd::Open)) return Mrl(value(LineCmd::Open));
        const auto list = m_parser.positionalArguments();
        return list.isEmpty() ? Mrl() : Mrl(list.first());
    }
private:
    auto addOption(LineCmd cmd, const QStringList &names, const QString &desc,
                   const QString &valName = QString(),
                   const QString &def = QString()) -> void;
    QMap<LineCmd, QCommandLineOption> m_options;
    QCommandLineParser m_parser;
};

CommandParser::CommandParser()
{
    m_parser.addHelpOption();
    m_parser.addVersionOption();
    const auto desc = u"The file path or URL to open."_q;
    m_parser.addPositionalArgument(u"mrl"_q, desc, u"mrl"_q);
}

auto CommandParser::addOption(LineCmd cmd, const QStringList &names, const QString &desc,
                              const QString &valName, const QString &def) -> void
{
    if (desc.contains(_L("%1")))
        m_parser.addOption(*m_options.insert(cmd, QCommandLineOption{ names, desc.arg('<'_q % valName % '>'_q), valName, def }));
    else
        m_parser.addOption(*m_options.insert(cmd, QCommandLineOption{ names, desc, valName, def }));
}

struct App::Data {
    Data(App *p): p(p), connection(_L("net.xylosper.bomi"), nullptr) {}

    App *p = nullptr;
    bool gldebug = false;
    QStringList styleNames;
    QString styleName = qApp->style()->objectName();
    bool unique = true, useLocalConfig = Global::useLocalConfig;
    QMap<QString, QVariant> openFolders;
    struct { Mrl mrl; QString sub; auto clear() { mrl = {}; sub.clear(); } } pended;
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
    QFont fixedFont = OS::defaultFixedFont();
    LocalConnection connection;
    CommandParser *parser = nullptr;

    auto open(const Mrl &mrl, const QString &sub) -> void
    {
        if (!main || !main->isSceneGraphInitialized())
            pended = { mrl, sub };
        else if (!mrl.isEmpty())
            main->openFromFileManager(mrl, sub);
        else if (!sub.isEmpty())
            main->setSubtitle(sub);
    }

    auto import() -> void
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

        const auto config = _WritablePath(Location::Config, false);
        const auto old = _OldConfigPath();
        if (config != old && !QFile::exists(config % u"/pref.json"_q)
                && QFile::exists(old % u"/pref.json"_q)) {
            QDir().mkpath(config);
            auto files = QDir(old).entryInfoList(QDir::Files);
            for (auto &file : files)
                copy(file.absoluteFilePath(), config % '/'_q % file.fileName());
        }

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
    }

    auto copyConfig(const QString &from, const QString &to) -> void
    {
        if (to != from) {
            QDir().mkpath(to);
            auto files = QDir(from).entryInfoList(QDir::Files);
            for (auto &file : files) {
                const QString dest = to % '/'_q % file.fileName();
                qDebug() << "remove" << dest << QFile::remove(dest);
                qDebug() << "copy" << file.absoluteFilePath() << dest << QFile::copy(file.absoluteFilePath(), dest);
            }
        }
    }
};

App::App(int &argc, char **argv)
: QApplication(argc, argv), d(new Data(this)) {
    if (QFile::exists(applicationDirPath() % "/bomi.ini"_a)) {
        QSettings set(applicationDirPath() % "/bomi.ini"_a, QSettings::IniFormat);
        d->useLocalConfig = set.value(u"app/use-local-config"_q, false).toBool();
        Global::useLocalConfig = set.value(u"global/use-local-config"_q, false).toBool();
        if (d->useLocalConfig != Global::useLocalConfig) {
            const auto from = _WritablePath(Location::Config, false);
            Global::useLocalConfig = d->useLocalConfig;
            const auto to = _WritablePath(Location::Config, false);
            d->copyConfig(from, to);
            set.setValue(u"global/use-local-config"_q, Global::useLocalConfig);
        }
    }

#ifdef Q_OS_LINUX
    setlocale(LC_NUMERIC,"C");
#endif

    OS::initialize();

    _New(d->parser);
    d->parser->addOption(LineCmd::Open, u"open"_q,
                         u"Open given %1 for file path or URL."_q, u"mrl"_q);
    d->parser->addOption(LineCmd::SetSubtitle, u"set-subtitle"_q,
                         u"Set subtitle file to display."_q, u"file"_q);
//    d->parser->addOption(LineCmd::AddSubtitle, u"add-subtitle"_q,
//                         u"Add subtitle file to display."_q, u"file"_q);
    d->parser->addOption(LineCmd::Wake, u"wake"_q,
                         u"Bring the application window in front."_q);
    d->parser->addOption(LineCmd::Action, u"action"_q,
                         u"Exectute %1 action or open %1 menu."_q, u"id"_q);
    d->parser->addOption(LineCmd::LogLevel, u"log-level"_q,
                         u"Maximum verbosity for log. %1 should be one of nexts:\n    "_q
                         % Log::levelNames().join(u", "_q), u"lv"_q);
    d->parser->addOption(LineCmd::Debug, u"debug"_q,
                         u"Turn on options for debugging."_q);
    d->parser->addOption(LineCmd::DumpApiTree, u"dump-api-tree"_q,
                         u"Dump API structure tree to stdout."_q);
    d->parser->addOption(LineCmd::DumpActionList, u"dump-action-list"_q,
                         u"Dump executable action list to stdout."_q);
#ifdef Q_OS_WIN
    d->parser->addOption(LineCmd::WinAssoc, u"win-assoc"_q,
                         u"Associate given comma-separated extension list."_q, u"ext"_q);
    d->parser->addOption(LineCmd::WinUnassoc, u"win-unassoc"_q,
                         u"Unassociate all extensions."_q);
    d->parser->addOption(LineCmd::WinAssocDefault, u"win-assoc-default"_q,
                         u"Associate default extensions."_q);
#endif
    d->parser->parse(arguments());
    d->gldebug = d->parser->isSet(LineCmd::Debug);
    const auto lvStdOut = d->parser->stdoutLogLevel();

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
    RootMenu::finalize();
    delete d->parser;
}

auto App::version() -> const char*
{
    return "0.9.11";
}

auto _CommonExtList(ExtTypes ext) -> QStringList;

auto App::executeToQuit() -> bool
{
    bool done = false;
    auto isSet = [&] (LineCmd cmd) {
        const auto set = d->parser->isSet(cmd);
        done |= set; return set;
    };
    if (isSet(LineCmd::DumpApiTree))
        AppObject::dumpInfo();
    if (isSet(LineCmd::DumpActionList))
        RootMenu::dumpInfo();
    if (isSet(LineCmd::WinAssoc))
        OS::associateFileTypes(nullptr, true, d->parser->value(LineCmd::WinAssoc).split(','_q));
    if (isSet(LineCmd::WinAssocDefault))
        OS::associateFileTypes(nullptr, true, _CommonExtList(VideoExt | AudioExt));
    if (isSet(LineCmd::WinUnassoc))
        OS::unassociateFileTypes(nullptr, true);
    if (isUnique() && sendMessage(CommandLine, d->parser->toJson())) {
        done = true;
        _Info("Another instance of bomi is already running. Exit this...");
    }
    return done;
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
        d->parser->parse(_FromJson<QStringList>(contents));
        runCommands();
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
    d->main->setIcon(defaultIcon());
#endif
    connect(d->main, &MainWindow::sceneGraphInitialized, this, [this] () {
        if (!d->pended.mrl.isEmpty())
            d->main->openFromFileManager(d->pended.mrl, d->pended.sub);
        d->pended.clear();
    }, Qt::QueuedConnection);
}

auto App::setWindowTitle(QWidget *w, const QString &title) -> void
{
    _Trace("Set window title of %% to '%%'.",
           w->metaObject()->className(), title);
    const QString text = title % (title.isEmpty() ? u""_q : u" - "_q)
                         % displayName();
    w->setWindowTitle(text);
}

auto App::setWindowTitle(QWindow *w, const QString &title) -> void
{
    _Trace("Set window title of %% to '%%'.",
           w->metaObject()->className(), title);
    const QString text = title % (title.isEmpty() ? u""_q : u" - "_q)
                         % displayName();
    w->setTitle(text);
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
        d->open(Mrl(ev->url().toString()), QString());
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
//    if (isSet(LineCmd::OpenGLDebug))
//        gldebug = true;
    if (!d->main)
        return;
    if (d->parser->isSet(LineCmd::Wake))
        d->main->wake();
    const auto mrl = d->parser->mrl();
    const auto sub = d->parser->value(LineCmd::SetSubtitle);
    d->open(mrl, sub);
    const auto args = d->parser->values(LineCmd::Action);
    if (!args.isEmpty())
        RootMenu::instance().execute(args[0]);
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

auto App::setUseLocalConfig(bool local) -> void
{
    if (_Change(d->useLocalConfig, local)) {
        QSettings set(applicationDirPath() % "/bomi.ini"_a, QSettings::IniFormat);
        set.setValue(u"app/use-local-config"_q, d->useLocalConfig);
    }
}

auto App::useLocalConfig() const -> bool
{
    return d->useLocalConfig;
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
