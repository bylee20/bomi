#ifndef LOG_HPP
#define LOG_HPP

#include "stdafx.hpp"
#include <tuple>

class App;

namespace Log {

enum LogLevel { Fatal, Error, Warn, Info, Debug, Trace };

extern LogLevel MaxLogLevel;

static inline QStringList logLevelsForOption() {
	return QStringList() << "fatal" << "error" << "warn" << "info" << "debug" << "trace";
}

static inline LogLevel logLevelFromOption(const QString &option) {
	const int index = logLevelsForOption().indexOf(option);
	return index < 0 ? Info : (LogLevel)index;
}

void setLogLevel(LogLevel lv);
LogLevel logLevel();

#define _ByteArrayLiteral(data) QByteArray(data, sizeof(data))

static inline QByteArray _ToLog(char n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(qint8 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(qint16 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(qint32 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(qint64 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(quint8 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(quint16 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(quint32 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(quint64 n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(float n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(double n) { return QByteArray::number(n); }
static inline QByteArray _ToLog(const QString &str) { return str.toLocal8Bit(); }
static inline QByteArray _ToLog(const char *str) { return QByteArray(str); }
static inline QByteArray _ToLog(bool b) { return b ? _ByteArrayLiteral("true") : _ByteArrayLiteral("false"); }


template<int idx, int size, bool go = idx < size>
struct StaticFor { };

template<int idx, int size>
struct StaticFor<idx, size, true> {
	static_assert(idx < size, "Wrong index");
	template<class... Args, class F>
	static inline void run(const std::tuple<Args...> &tuple, const F &func) {
		func(std::get<idx>(tuple));
		StaticFor<idx+1, size>::run(tuple, func);
	}
};

template<int idx, int size>
struct StaticFor<idx, size, false> {
	static_assert(idx >= size, "Wrong index");
	template<class T, class F>
	static inline void run(const T &, const F &) { }
	template<class T, class F>
	static inline void run(T &&, const F &) { }
};

struct ToLog {
	template<class... Args>
	ToLog(const QString &format, const std::tuple<Args...> &tuple): m_format(format) {
		using Tuple = std::tuple<Args...>;
		StaticFor<0, std::tuple_size<Tuple>::value>::run(tuple, *this);
		flush();
	}
	template<typename... Args>
	ToLog(const QString &format, const Args &... args): ToLog(format, std::tie(args...)) { }
	template<typename T>
	inline void operator () (const T &t) const {
		const int to = m_format.indexOf(QLatin1String("%%"), m_pos);
		if (Q_UNLIKELY(to < 0)) {
			qDebug() << "Wrong log format";
			return;
		}
		m_log += m_format.midRef(m_pos, to-m_pos).toLocal8Bit();
		m_log += _ToLog(t);
		m_pos = to + 2;
	}
	const QByteArray &log() const { return m_log; }
private:
	void flush() { m_log += m_format.midRef(m_pos).toLocal8Bit(); m_pos = m_format.size(); }
	const QString &m_format;
	mutable int m_pos = 0;
	mutable QByteArray m_log;
};

template<typename F>
static inline void write(const char *context, LogLevel lv, const QString &fmt, const F &make_tuple) {
	if (lv <= MaxLogLevel) {
		qDebug("[%s] %s", context, ToLog(fmt, make_tuple()).log().constData());
		if (lv == Fatal)
			exit(1);
	}
}

//							for (; m_pos<m_format.size(); ++m_pos) {
//								const char c = m_format.at(m_pos);
//								if (c == '%' && m_pos+1 < m_format.size() && m_format.at(m_pos+1) == '%') {
//									m_log += _ToLog(t);
//									m_pos += 2;
//									return;
//								} else
//									m_log.push_back(c);
//							}
}

#define DECLARE_LOG_CONTEXT(ctx) static inline const char *getLogContext() { return (#ctx); }
#define _Fatal(fmt, ...) ::Log::write(getLogContext(), ::Log::Fatal, fmt, [&] { return std::make_tuple(__VA_ARGS__); })
#define _Error(fmt, ...) ::Log::write(getLogContext(), ::Log::Error, fmt, [&] { return std::make_tuple(__VA_ARGS__); })
#define _Warn(fmt, ...) ::Log::write(getLogContext(), ::Log::Warn, fmt, [&] { return std::make_tuple(__VA_ARGS__); })
#define _Info(fmt, ...) ::Log::write(getLogContext(), ::Log::Info, fmt, [&] { return std::make_tuple(__VA_ARGS__); })
#define _Debug(fmt, ...) ::Log::write(getLogContext(), ::Log::Debug, fmt, [&] { return std::make_tuple(__VA_ARGS__); })
#define _Trace(fmt, ...) ::Log::write(getLogContext(), ::Log::Trace, fmt, [&] { return std::make_tuple(__VA_ARGS__); })

#endif // LOG_HPP
