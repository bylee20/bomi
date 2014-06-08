#ifndef STDAFX_HPP
#define STDAFX_HPP

#if defined(__cplusplus) && !defined(__OBJC)

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQuick>
#include <QtSql>
#include <QtXml>
#include <set>
#include <sys/time.h>
#include <qmath.h>
#include <type_traits>
#include <array>
#include <iterator>
#include <tuple>
#include <cmath>
#include <cstdint>
#include <deque>
#include <algorithm>

#ifdef Q_OS_LINUX
    #include <QX11Info>
    #include <QtDBus>
#endif // Q_OS_LINUX
#ifdef Q_OS_MAC
    #include <QtMacExtras>
#endif // Q_OS_MAC

template<class State, class... Args>
using Signal = void(State::*)(Args...);

#define SIA static inline auto
#define SCA static constexpr auto
#define SCIA static constexpr inline auto
#define CIA constexpr inline auto

SCA QCS = Qt::CaseSensitive;
SCA QCI = Qt::CaseInsensitive;

using QRegEx = QRegularExpression;
using QRegExMatch = QRegularExpressionMatch;
using QRegExMatchIterator = QRegularExpressionMatchIterator;

namespace Pch {

SIA operator "" _q(const char16_t *str, size_t len) -> QString
{ return QString::fromRawData(reinterpret_cast<const QChar*>(str), len); }

SIA operator "" _a(const char *str, size_t len) -> QLatin1String
{ return QLatin1String(str, len); }

SIA operator ""_b(const char *str, size_t len) -> QByteArray
{ return QByteArray::fromRawData(str, len); }

SIA operator "" _8(const char *str, size_t len) -> QString
{ return QString::fromUtf8(str, len); }

SCIA operator "" _q(char16_t c) -> QChar { return QChar((ushort)c); }

SCIA operator "" _q(char c) -> QChar { return ushort(uchar(c)); }

template<class... Args>
using T = std::tuple<Args...>;

template<class... Args>
SCIA _T(const Args&... args) -> T<Args...>
{ return std::tuple<Args...>(args...); }

SIA _L(const char *str) -> QLatin1String
{ return QLatin1String(str); }

SIA _N(int n, int base, int width, const QChar &c = ' '_q) -> QString
{ return u"%1"_q.arg(n, width, base, c); }

SIA _N(int n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(quint32 n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(quint64 n, int base = 10) -> QString
{ return QString::number(n, base); }

SIA _N(double n, int dec = 1) -> QString
{ return QString::number(n, 'f', dec); }

SIA _N(double n, int dec, int width,
                      const QChar &c = QChar(QChar::Nbsp)) -> QString
{ return u"%1"_q.arg(n, width, 'f', dec, c); }

template<class Conv, class Container>
SIA _ToStringList(const Container &c, Conv f) -> QStringList
{ QStringList list; for (auto &t : c) list.push_back(f(t)); return list; }

// number with sign
template<class N>
SIA _SignSymbol(N n) -> QChar { return n < 0 ? '-'_q : n > 0 ? '+'_q : u'±'_q; }

SIA _NS(int n) -> QString
{ return _SignSymbol(n) % _N(qAbs(n)); }

SIA _NS(double n, int dec = 1) -> QString
{ return _SignSymbol(n) % _N(qAbs(n), dec); }

template<class T>
SIA _InRange(const T &min, const T &val, const T &max) -> bool
{ return min <= val && val <= max; }

template<class T, class S>
SCIA _IsOneOf(const T &t, const S &s) -> bool { return t == s; }

template<class T, class S, class U, class... Args>
SCIA _IsOneOf(const T &t, const S &s, const U &u, const Args&... args) -> bool
{ return t == s || _IsOneOf(t, u, args...); }

template <class T>
SIA _Change(T &the, const T &one) -> bool
{ if (the != one) {the = one; return true;} return false; }

template<class T>
SIA _C(T& t) -> const T& { return t; }

SIA _MSecToTime(int ms) -> QTime { return QTime::fromMSecsSinceStartOfDay(ms); }

SIA _MSecToString(int ms, const QString &fmt = u"hh:mm:ss"_q) -> QString
{ return _MSecToTime(ms).toString(fmt); }

enum ExtType {
    AllExt       = 0,
    AudioExt    = 1 << 0,
    VideoExt    = 1 << 1,
    SubtitleExt = 1 << 2,
    ImageExt    = 1 << 3,
    DiscExt     = 1 << 4,
    PlaylistExt = 1 << 5,

    MediaExt    = AudioExt | VideoExt | ImageExt
};

Q_DECLARE_FLAGS(ExtTypes, ExtType)
Q_DECLARE_OPERATORS_FOR_FLAGS(ExtTypes)

auto _ToNameFilter(ExtTypes exts) -> QStringList;
auto _ToFilter(ExtTypes exts) -> QString;
auto _IsSuffixOf(ExtType ext, const QString &suffix) -> bool;
auto _ExtList(ExtType ext) -> QStringList;

auto _GetOpenFiles(QWidget *parent, const QString &title, ExtTypes exts,
                       const QString &key = QString()) -> QStringList;

auto _GetOpenFile(QWidget *parent, const QString &title, ExtTypes exts,
                      const QString &key = QString()) -> QString;

auto _GetSaveFile(QWidget *parent, const QString &title,
                      const QString &fileName, ExtTypes exts,
                      const QString &key = QString()) -> QString;

auto _GetOpenDir(QWidget *parent, const QString &title,
                 const QString &key = QString()) -> QString;

auto _SetLastOpenPath(const QString &path, const QString &k = QString()) -> void;

auto _LastOpenPath(const QString &key = QString()) -> QString;

template<class T>
SIA _QmlSingleton(QQmlEngine *, QJSEngine *) -> QObject* { return new T; }

template<class T, typename S = T>
constexpr SIA _Max() -> S { return (S)std::numeric_limits<T>::max(); }

template<class T, typename S = T>
constexpr SIA _Min() -> S { return (S)std::numeric_limits<T>::min(); }

template<class T, typename... Args>
SIA _New(T *&t, Args... args) -> T* { return (t = new T(args...)); }

template<class T, typename... Args>
SIA _Renew(T *&t, Args... args) -> T* {delete t; return (t = new T(args...)); }

template<class T>
SIA _Delete(T *&t) -> void { if (t) { delete t; t = nullptr; } }

SIA _SystemTime() -> quint64
{ struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }

template<class T>
SIA _Expand(T &t, int size, double extra = 1.2) -> bool
{
    if (uint(size) < uint(t.size()))
        return false;
    t.resize(size*extra);
    return true;
}

auto _Uncompress(const QByteArray &data) -> QByteArray;

SIA _JsonToString(const QJsonObject &json,
                   QJsonDocument::JsonFormat format = QJsonDocument::Compact)
-> QString { return QString::fromUtf8(QJsonDocument(json).toJson(format)); }

SIA _JsonFromString(const QString &str) -> QJsonObject
    { return QJsonDocument::fromJson(str.toUtf8()).object(); }

SIA _WritablePath(QStandardPaths::StandardLocation loc
                       = QStandardPaths::DataLocation) -> QString
{
    auto path = QStandardPaths::writableLocation(loc);
    if (!QDir().mkpath(path))
        return QString();
    return path;
}

SIA _JsonToInt(const QJsonValue &val) -> qlonglong
{ return std::llround(val.toDouble()); }
template<class T>
SIA _JsonFromInt(T t) -> QJsonValue { return static_cast<double>(t); }

}

using namespace Pch;

#endif

#endif // STDAFX_HPP
