#include "misc/log.hpp"

Log::Level Log::m_maxLevel = Log::Info;

const QStringList Log::m_options = QStringList()
        << u"fatal"_q << u"error"_q << u"warn"_q
        << u"info"_q  << u"debug"_q << u"trace"_q;
