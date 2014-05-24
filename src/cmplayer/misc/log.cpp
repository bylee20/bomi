#include "misc/log.hpp"

Log::Level Log::m_maxLevel = Log::Info;

const QStringList Log::m_options = QStringList()
        << "fatal" << "error" << "warn" << "info" << "debug" << "trace";
