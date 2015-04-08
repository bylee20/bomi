#include "log.hpp"
#include "dataevent.hpp"
#include "logoption.hpp"
#include "configure.hpp"
#include "tmp/algorithm.hpp"
#include <QTextCodec>
#include <QBuffer>
#include <cstdlib>

#if HAVE_SYSTEMD
#include <syslog.h>
#include <systemd/sd-journal.h>
static const std::array<int, 7> jp = [] () {
    std::array<int, 7> ret;
    ret[Log::Fatal] = LOG_CRIT;
    ret[Log::Error] = LOG_ERR;
    ret[Log::Warn] = LOG_WARNING;
    ret[Log::Info] = LOG_INFO;
    ret[Log::Debug] = LOG_DEBUG;
    ret[Log::Trace] = LOG_DEBUG;
    return ret;
}();
#endif

static Log::Level lvStdOut  = Log::Trace;
static Log::Level lvStdErr  = Log::Off;
static Log::Level lvJournal = Log::Off;
static Log::Level lvFile    = Log::Off;
static Log::Level lvViewer  = Log::Off;
static Log::Level lvMax     = Log::Trace;

static QReadWriteLock s_rwLock;
static QHash<QObject*, int> s_subscribers;

static QSharedPointer<FILE> s_file;
static bool s_local8BitIsUtf8 = false;

SIA encodeForTerminal(const QByteArray &log) -> QByteArray
{
#ifndef Q_OS_WIN
    if (s_local8BitIsUtf8)
        return log;
#endif
    return QString::fromUtf8(log).toLocal8Bit();
}

const QStringList Log::m_options = QStringList()
        << u"off"_q   << u"fatal"_q << u"error"_q << u"warn"_q
        << u"info"_q  << u"debug"_q << u"trace"_q;

SIA print(FILE *file, const QByteArray &log) -> void
{
    fwrite(log.constData(), 1, log.size(), file);
    fflush(file);
}

auto Log::print(Level lv, const QByteArray &log) -> void
{
#if HAVE_SYSTEMD
    if (lv <= lvJournal)
        sd_journal_print(jp[lv], log.constData());
#endif
    if (lv <= lvStdOut)
        ::print(stdout, encodeForTerminal(log));
    if (lv <= lvStdErr)
        ::print(stderr, encodeForTerminal(log));
    if (lv <= lvFile && s_file)
        ::print(s_file.data(), log);
    if (lv <= lvViewer && !s_subscribers.isEmpty()) {
        s_rwLock.lockForRead();
        auto &s = _C(s_subscribers);
        auto str = QString::fromUtf8(log); str.chop(1);
        for (auto it = s.begin(); it != s.end(); ++it)
            _PostEvent(it.key(), it.value(), lv, str);
        s_rwLock.unlock();
    }
    if (lv == Fatal)
        abort();
}

static const std::array<Log::Level, 4> lvQt = []() {
    std::array<Log::Level, 4> ret;
    ret[QtDebugMsg] = Log::Debug;
    ret[QtWarningMsg] = Log::Warn;
    ret[QtCriticalMsg] = Log::Error;
    ret[QtFatalMsg] = Log::Fatal;
    return ret;
}();

auto Log::qt(QtMsgType type, const QMessageLogContext &, const QString &msg) -> void
{
    write("Qt", lvQt[type], msg.toUtf8());
}

static LogOption s_option;

auto Log::setOption(const LogOption &option) -> void
{
    s_option = option;
    // no thread safety!! should be called only once on initalization
    lvStdOut  = option.level(LogOutput::StdOut);
    lvStdErr  = option.level(LogOutput::StdErr);
    lvJournal = option.level(LogOutput::Journal);
    lvFile    = option.level(LogOutput::File);
    lvViewer  = option.level(LogOutput::Viewer);
    lvMax = tmp::max(lvStdOut, lvStdErr, lvFile, lvViewer);
#if HAVE_SYSTEMD
    lvMax = tmp::max(lvMax, lvJournal);
#endif

    s_local8BitIsUtf8 = QTextCodec::codecForLocale()->mibEnum() == 106;

    if (!lvFile)
        return;
    auto path = option.file().toLocal8Bit();
    auto pf = fopen(path.constData(), "a");
    if (!pf) {
        qDebug("Cannot open file: %s\n", path.constData());
        return;
    }
    s_file = QSharedPointer<FILE>(pf, fclose);
}

auto Log::option() -> const LogOption&
{
    return s_option;
}

auto Log::maximumLevel() -> Level
{
    return lvMax;
}

auto Log::subscribe(QObject *o, int event) -> int
{
    QWriteLocker l(&s_rwLock);
    s_subscribers.insert(o, event);
    return s_option.lines() ? s_option.lines() : _Max<int>();
}

auto Log::unsubscribe(QObject *o) -> void
{
    s_rwLock.lockForWrite();
    s_subscribers.remove(o);
    s_rwLock.unlock();
}

auto _ToLog(const QVariant &var) -> QByteArray
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    QDebug(&buffer) << var;
    QByteArray ret = std::move(buffer.buffer());
    buffer.close();
    ret.chop(1);
    return ret;
}
