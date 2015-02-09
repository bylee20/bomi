#include "misc/log.hpp"
#include "misc/logoption.hpp"
#include "tmp/algorithm.hpp"
#include <cstdlib>
#include <syslog.h>
#include <systemd/sd-journal.h>

static Log::Level lvStdOut  = Log::Trace;
static Log::Level lvStdErr  = Log::Off;
static Log::Level lvJournal = Log::Off;
static Log::Level lvFile    = Log::Off;
static Log::Level lvMax     = Log::Trace;

QSharedPointer<FILE> file;

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
    if (lv <= lvJournal)
        sd_journal_print(jp[lv], log.constData());
    if (lv <= lvStdOut)
        ::print(stdout, log);
    if (lv <= lvStdErr)
        ::print(stderr, log);
    if (lv <= lvFile && ::file)
        ::print(file.data(), log);
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
    write("Qt", lvQt[type], msg.toLocal8Bit());
}

auto Log::setOption(const LogOption &option) -> void
{
    // no thread safety!! should be called only once on initalization
    lvStdOut  = option.level(LogOutput::StdOut);
    lvStdErr  = option.level(LogOutput::StdErr);
    lvJournal = option.level(LogOutput::Journal);
    lvFile    = option.level(LogOutput::File);
    lvMax = tmp::max(lvStdOut, lvStdErr, lvJournal, lvFile);

    if (!lvFile)
        return;
    auto path = option.file().toLocal8Bit();
    auto pf = fopen(path.constData(), "a");
    if (!pf) {
        qDebug("Cannot open file: %s\n", path.constData());
        return;
    }
    ::file = QSharedPointer<FILE>(pf, fclose);
}

auto Log::maximumLevel() -> Level
{
    return lvMax;
}
