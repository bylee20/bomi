#include "misc/log.hpp"
#include <cstdlib>
#include <syslog.h>
#include <systemd/sd-journal.h>

Log::Level Log::m_maxLevel = Log::Info;

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

auto Log::print(Level lv, const QByteArray &log) -> void
{
    sd_journal_print(jp[lv], log.constData());

    if (lv > m_maxLevel)
        return;

    std::fwrite(log.constData(), 1, log.size(), stdout);
    std::fflush(stdout);


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
