#ifndef LOG_HPP
#define LOG_HPP

#include "stdafx.hpp"

class App;

namespace Log {

enum LogLevel { Fatal, Error, Warn, Info, Debug, Trace };

static inline QStringList logLevelsForOption() {
	return QStringList() << "fatal" << "error" << "warn" << "info" << "debug" << "trace";
}

static inline LogLevel logLevelFromOption(const QString &option) {
	const int index = logLevelsForOption().indexOf(option);
	return index < 0 ? Info : (LogLevel)index;
}

static inline QString _ToLog(char n) { return QString::number(n); }
static inline QString _ToLog(qint8 n) { return QString::number(n); }
static inline QString _ToLog(qint16 n) { return QString::number(n); }
static inline QString _ToLog(qint32 n) { return QString::number(n); }
static inline QString _ToLog(qint64 n) { return QString::number(n); }
static inline QString _ToLog(quint8 n) { return QString::number(n); }
static inline QString _ToLog(quint16 n) { return QString::number(n); }
static inline QString _ToLog(quint32 n) { return QString::number(n); }
static inline QString _ToLog(quint64 n) { return QString::number(n); }
static inline QString _ToLog(float n) { return QString::number(n); }
static inline QString _ToLog(double n) { return QString::number(n); }
static inline QString _ToLog(const QString &str) { return str; }
static inline QString _ToLog(const char *str) { return QLatin1String(str); }
static inline QString _ToLog(bool b) { return b ? QStringLiteral("true") : QStringLiteral("false"); }

class Helper {
public:
	template<typename... Args>
	inline Helper(const QString &fmt, const Args &... args) {
		write(fmt, args...);
	}
	static inline LogLevel level() { return m_logLevel; }
	inline const QString &log() const { return m_log; }
private:
	inline void write(const QString &fmt) { m_log += fmt.midRef(m_pos); }
	template<typename T, typename... Args>
	inline void write(const QString &fmt, T &&t, const Args &... args) {
		const int to = fmt.indexOf(m_ph, m_pos);
		if (Q_UNLIKELY(to < 0)) {
			qDebug() << "Wrong log format";
			return;
		}
		m_log += fmt.midRef(m_pos, to-m_pos);
		m_log += _ToLog(t);
		m_pos = to + 2;
		write(fmt, args...);
	}
	static const QLatin1String m_ph;
	static LogLevel m_logLevel;
	QString m_log;
	int m_pos = 0;
	friend class ::App;
};

template<typename... Args>
static inline void _Log(LogLevel level, const QString &fmt, const Args &... args) {
	if (level <= Helper::level())
		qDebug(qPrintable(Helper(fmt, args...).log()));
}

template<typename... Args>
static inline void _Log(const QString &context, LogLevel level, const QString &fmt, const Args &... args) {
	if (level <= Helper::level()) {
		qDebug("[%s] %s", qPrintable(context), qPrintable(Helper(fmt, args...).log()));
		if (level == Fatal)
			exit(1);
	}
}

#define DEC_LOG_FUNC(ctx, lv) template<typename... Args> static inline void _##lv(const QString &fmt, const Args &... args) { Log::_Log(QStringLiteral(#ctx), Log::lv, fmt, args...); }
#define DEC_LOG_FUNCS(ctx) \
DEC_LOG_FUNC(ctx, Fatal) \
DEC_LOG_FUNC(ctx, Error) \
DEC_LOG_FUNC(ctx, Warn) \
DEC_LOG_FUNC(ctx, Info) \
DEC_LOG_FUNC(ctx, Debug) \
DEC_LOG_FUNC(ctx, Trace)

template<typename... Args>
static inline void _Info(const QString &fmt, const Args &... args) { _Log(Info, fmt, args...); }
template<typename... Args>
static inline void _Fatal(const QString &fmt, const Args &... args) { _Log(Fatal, fmt, args...); }


}

#endif // LOG_HPP
