#ifndef LOG_HPP
#define LOG_HPP

#include "stdafx.hpp"

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
static inline QByteArray _ToLog(const QStringRef &str) { return str.toLocal8Bit(); }
static inline QByteArray _ToLog(const char *str) { return QByteArray(str); }
static inline QByteArray _ToLog(const QByteArray &str) { return str; }
static inline QByteArray _ToLog(bool b) { return b ? "true" : "false"; }

class Log {
public:
	enum Level { Fatal, Error, Warn, Info, Debug, Trace };
	static inline Level maximumLevel() { return m_maxLevel; }
	static inline void setMaximumLevel(Level level) { m_maxLevel = level; }
	template<typename F>
	static void write(const char *context, Level level, const F &getLogText) {
		if (level <= m_maxLevel)
			print(context, level, getLogText());
	}
	template<class... Args>
	static inline void write(const char *context, Level level, const QByteArray &format, const Args &... args) {
		if (level <= m_maxLevel)
			print(context, level, Helper(format, args...).log());
	}
	template<class... Args>
	static inline void write(Level level, const QByteArray &format, const Args &... args) {
		if (level <= m_maxLevel)
			print(level, Helper(format, args...).log());
	}
	template<typename... Args>
	static inline QByteArray parse(const QByteArray &format, const Args &... args) { return Helper(format, args...).log(); }
	static inline QStringList options() { return m_options; }
	static inline void setMaximumLevel(const QString &option) {
		const int index = m_options.indexOf(option);
		setMaximumLevel(index < 0 ? Info : (Level)index);
	}
private:
	static inline void print(const char *context, Level level, const QByteArray &log) {
		qDebug("[%s] %s", context, log.constData());
		if (level == Fatal)
			exit(1);
	}
	static inline void print(Level level, const QByteArray &log) {
		qDebug("%s", log.constData());
		if (level == Fatal)
			exit(1);
	}
	struct Helper {
		template<typename... Args>
		inline Helper(const QByteArray &format, const Args &... args): m_format(format) { write(args...); }
		QByteArray log() const { return m_log; }
	private:
		inline void write() { m_log.append(m_format.data() + m_pos, m_format.size()-m_pos); }
		template<class T, class... Args>
		inline void write(const T &t, const Args &... args) {
			for (; m_pos<m_format.size(); ++m_pos) {
				const char c = m_format.at(m_pos);
				if (c == '%' && m_pos+1 < m_format.size() && m_format.at(m_pos+1) == '%') {
					m_log += _ToLog(t);
					m_pos += 2;
					break;
				} else
					m_log.push_back(c);
			}
			if (m_pos > m_format.size()) {
				qDebug("Wrong log placeholders");
				return;
			}
			write(args...);
		}
		const QByteArray &m_format;
		mutable int m_pos = 0;
		mutable QByteArray m_log;
	};
	static Level m_maxLevel;
	static const QStringList m_options;
};

#define DECLARE_LOG_CONTEXT(ctx) static inline const char *getLogContext() { return (#ctx); }
#define _WRITE_LOG(lv, fmt, ...) Log::write(getLogContext(), Log::lv , [&]() { return Log::parse(fmt, ##__VA_ARGS__); })
#define _Fatal(fmt, ...) _WRITE_LOG(Fatal, fmt, ##__VA_ARGS__)
#define _Error(fmt, ...) _WRITE_LOG(Error, fmt, ##__VA_ARGS__)
#define _Warn(fmt, ...) _WRITE_LOG(Warn, fmt, ##__VA_ARGS__)
#define _Info(fmt, ...) _WRITE_LOG(Info, fmt, ##__VA_ARGS__)
#define _Debug(fmt, ...) _WRITE_LOG(Debug, fmt, ##__VA_ARGS__)
#define _Trace(fmt, ...) _WRITE_LOG(Trace, fmt, ##__VA_ARGS__)

#endif // LOG_HPP
