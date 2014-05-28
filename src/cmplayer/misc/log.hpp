#ifndef LOG_HPP
#define LOG_HPP

SIA _ToLog(char n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(qint8 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(qint16 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(qint32 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(qint64 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(quint8 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(quint16 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(quint32 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(quint64 n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(float n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(double n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(const QString &str) -> QByteArray { return str.toLocal8Bit(); }
SIA _ToLog(const QStringRef &str) -> QByteArray { return str.toLocal8Bit(); }
SIA _ToLog(const char *str) -> QByteArray { return QByteArray(str); }
SIA _ToLog(const QByteArray &str) -> QByteArray { return str; }
SIA _ToLog(bool b) -> QByteArray { return b ? "true" : "false"; }
SIA _ToLog(const void *ptr) -> QByteArray
    { return "0x" + QByteArray::number(reinterpret_cast<quintptr>(ptr), 16); }
SIA _ToLog(const QObject *qt) -> QByteArray
{
    if (!qt) return "QObject(0x0)";
    return QByteArray(qt->metaObject()->className()) + "(0x"
           + QByteArray::number(reinterpret_cast<quintptr>(qt)) + ")";
}

class Log {
public:
    enum Level { Fatal, Error, Warn, Info, Debug, Trace };
    static auto maximumLevel() -> Level { return m_maxLevel; }
    static auto  setMaximumLevel(Level level) -> void { m_maxLevel = level; }
    template<class F>
    static auto write(const char *context, Level level, const F &getLogText) -> void
    {
        if (level <= m_maxLevel)
            print(context, level, getLogText());
    }
    template<class... Args>
    static auto write(const char *context, Level level, const QByteArray &format, const Args &... args) -> void {
        if (level <= m_maxLevel)
            print(context, level, Helper(format, args...).log());
    }
    template<class... Args>
    static auto write(Level level, const QByteArray &format, const Args &... args) -> void {
        if (level <= m_maxLevel)
            print(level, Helper(format, args...).log());
    }
    template<class... Args>
    static auto parse(const QByteArray &format, const Args &... args) -> QByteArray { return Helper(format, args...).log(); }
    static auto options() -> QStringList { return m_options; }
    static auto setMaximumLevel(const QString &option) -> void {
        const int index = m_options.indexOf(option);
        setMaximumLevel(index < 0 ? Info : (Level)index);
    }
private:
    static auto print(const char *context, Level level, const QByteArray &log) -> void {
        qDebug("[%s] %s", context, log.constData());
        if (level == Fatal)
            exit(1);
    }
    static auto print(Level level, const QByteArray &log) -> void {
        qDebug("%s", log.constData());
        if (level == Fatal)
            exit(1);
    }
    struct Helper {
        template<class... Args>
        inline Helper(const QByteArray &format, const Args &... args): m_format(format) { write(args...); }
        auto log() const -> QByteArray { return m_log; }
    private:
        inline auto write() -> void { m_log.append(m_format.data() + m_pos, m_format.size()-m_pos); }
        template<class T, class... Args>
        inline auto write(const T &t, const Args &... args) -> void {
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
