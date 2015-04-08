#ifndef LOG_HPP
#define LOG_HPP

struct LogOption;

SIA _ToLog(char n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(signed char n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(short n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(int n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(long n) -> QByteArray { return QByteArray::number((qlonglong)n); }
SIA _ToLog(long long n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(unsigned char n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(unsigned short n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(unsigned int n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(unsigned long n) -> QByteArray { return QByteArray::number((qulonglong)n); }
SIA _ToLog(unsigned long long n) -> QByteArray { return QByteArray::number(n); }

SIA _ToLog(float n) -> QByteArray { return QByteArray::number(n); }
SIA _ToLog(double n) -> QByteArray { return QByteArray::number(n); }

SIA _ToLog(const QString &str) -> QByteArray { return str.toUtf8(); }
SIA _ToLog(const QStringRef &str) -> QByteArray { return str.toUtf8(); }
SIA _ToLog(const char *str) -> QByteArray { return QByteArray(str); }
SIA _ToLog(const QByteArray &str) -> QByteArray { return str; }
SIA _ToLog(bool b) -> QByteArray { return b ? "true"_b : "false"_b; }
SIA _ToLog(const void *ptr) -> QByteArray
    { return "0x" + QByteArray::number(reinterpret_cast<quintptr>(ptr), 16); }
SIA _ToLog(const QObject *qt) -> QByteArray
{
    if (!qt) return "QObject(0x0)"_b;
    return QByteArray(qt->metaObject()->className())
           + '(' + _ToLog(static_cast<const void*>(qt)) + ')';
}
SIA _ToLog(const QSize &s) -> QByteArray
{ return _ToLog(s.width()) + 'x' + _ToLog(s.height()); }
SIA _ToLog(const QSizeF &s) -> QByteArray
{ return _ToLog(s.width()) + 'x' + _ToLog(s.height()); }
SIA _ToLog(const QPoint &p) -> QByteArray
{ return '(' + _ToLog(p.x()) + ", "_b + _ToLog(p.y()) + ')'; }
SIA _ToLog(const QPointF &p) -> QByteArray
{ return '(' + _ToLog(p.x()) + ", "_b + _ToLog(p.y()) + ')'; }
template<class T>
SIA _ToLog(const QList<T> &list) -> QByteArray
{
    if (list.isEmpty())
        return "QList()"_b;
    QByteArray log = "QList(";
    for (auto &t : list) {
        log += _ToLog(t);
        log += ',';
    }
    log[log.size() - 1] = ')';
    return log;
}
auto _ToLog(const QVariant &var) -> QByteArray;

class Log {
    constexpr static const char *l2t = " FEWIDT";
public:
    enum Level { Off, Fatal, Error, Warn, Info, Debug, Trace };
    template<class F>
    static auto write(Level level, F &&getLogText) -> void
    {
        if (level <= maximumLevel())
            print(level, std::move(getLogText() += '\n'));
    }
    template<class... Args>
    static auto write(const char *ctx, Level level, const QByteArray &format,
                      const Args &... args) -> void
    {
        if (level <= maximumLevel())
            print(level, std::move(Helper(level, ctx, format, args...).log() += '\n'));
    }
    template<class... Args>
    static auto parse(Level lv, const char *ctx, const QByteArray &fmt, const Args &... args) -> QByteArray
        { return std::move(Helper(lv, ctx, fmt, args...).log()); }
    template<class... Args>
    static auto parse(const QByteArray &fmt, const Args &... args) -> QByteArray
        { return std::move(Helper(fmt, args...).log()); }
    static auto name(Level level) -> QString { return m_options[level]; }
    static auto levelNames() -> QStringList { return m_options; }
    static auto level(const QString &name) -> Level
    {
        const int index = m_options.indexOf(name);
        return index < 0 ? Off : (Level)index;
    }
    static auto print(Level lv, const QByteArray &log) -> void;
    static auto maximumLevel() -> Level;
    static auto setOption(const LogOption &option) -> void;
    static auto option() -> const LogOption&;
    static auto qt(QtMsgType type, const QMessageLogContext &context, const QString &msg) -> void;
    static auto subscribe(QObject *o, int event) -> int;
    static auto unsubscribe(QObject *o) -> void;
private:
    struct Helper {
        template<class... Args>
        inline Helper(const QByteArray &format, const Args &... args)
            : m_format(format)
        {
            m_log.reserve(128);
            write(args...);
        }
        template<class... Args>
        inline Helper(Level lv, const QByteArray &format, const Args &... args)
            : m_format(format)
        {
            m_log.reserve(128);
            ((m_log += '(') += l2t[lv]) += ") ";
            write(args...);
        }
        template<class... Args>
        inline Helper(Level lv, const char *ctx, const QByteArray &format, const Args &... args)
            : m_format(format)
        {
            m_log.reserve(128);
            ((((m_log += '(') += l2t[lv]) += ")[") += ctx) += "] ";
            write(args...);
        }
        auto log() const -> QByteArray& { return m_log; }
    private:
        inline auto write() -> void {
            m_log.append(m_format.data() + m_pos, m_format.size()-m_pos);
        }
        template<class T, class... Args>
        auto write(const T &t, const Args &... args) -> void
        {
            for (; m_pos < m_format.size(); ++m_pos) {
                const char c = m_format.at(m_pos);
                if (c == '%' && m_pos + 1 < m_format.size()
                        && m_format.at(m_pos+1) == '%') {
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

#define DECLARE_LOG_CONTEXT(ctx) \
    static inline const char *getLogContext() { return (#ctx); }

#define _WriteLog(lv, fmt, ...) Log::write(lv, [&] () \
    { return std::move(Log::parse(lv, getLogContext(), fmt, ##__VA_ARGS__)); })
#define _Fatal(fmt, ...) _WriteLog(Log::Fatal, fmt, ##__VA_ARGS__)
#define _Error(fmt, ...) _WriteLog(Log::Error, fmt, ##__VA_ARGS__)
#define _Warn(fmt, ...)  _WriteLog(Log::Warn,  fmt, ##__VA_ARGS__)
#define _Info(fmt, ...)  _WriteLog(Log::Info,  fmt, ##__VA_ARGS__)
#define _Debug(fmt, ...) _WriteLog(Log::Debug, fmt, ##__VA_ARGS__)
#define _Trace(fmt, ...) _WriteLog(Log::Trace, fmt, ##__VA_ARGS__)

Q_DECLARE_METATYPE(Log::Level)

#endif // LOG_HPP
