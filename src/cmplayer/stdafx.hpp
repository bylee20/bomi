#ifndef STDAFX_HPP
#define STDAFX_HPP

#ifdef __cplusplus

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQuick>
#include <QtSql>
#include <QtXml>
#include <set>
#include <sys/time.h>
#include <qmath.h>
#ifndef __OBJC__
#include <type_traits>
#include <array>
#include <iterator>
#endif

#ifdef Q_OS_LINUX
#include <QX11Info>
#include <QtDBus>
#endif
#ifdef Q_OS_MAC
#include <QtMacExtras>
#endif

template<class State, class... Args>
using Signal = void(State::*)(Args...);

namespace Pch {
#define SIA static inline auto
SIA _L(const char *str) -> QLatin1String
{ return QLatin1String(str); }

SIA _L(char c) -> QLatin1Char
{ return QLatin1Char(c); }

SIA _U(const char *utf8, int len = -1) -> QString
{ return QString::fromUtf8(utf8, len); }

SIA _N(int n, int base, int width,
                      const QChar &c = _L(' ')) -> QString
{ return QString("%1").arg(n, width, base, c); }

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
{ return QString("%1").arg(n, width, 'f', dec, c); }

SIA _Chopped(const QString &str, int n) -> QString
{ QString ret = str; ret.chop(n); return ret; }

// number with sign
template<class N>
SIA _SignSymbol(N n) -> QString
{ return n < 0 ? _L("-") : (n > 0 ? _L("+") : _U("±")); }

SIA _NS(int n) -> QString
{ return _SignSymbol(n) % _N(qAbs(n)); }

SIA _NS(double n, int dec = 1) -> QString
{ return _SignSymbol(n) % _N(qAbs(n), dec); }

template<class T>
SIA _InRange(const T &min, const T &val, const T &max) -> bool
{ return min <= val && val <= max; }

SIA _IsNumber(ushort c) -> bool
{return _InRange<ushort>('0', c, '9');}

SIA _IsAlphabet(ushort c) -> bool
{return _InRange<ushort>('a', c, 'z') || _InRange<ushort>('A', c, 'Z');}

SIA _IsAlphabet(const QString &text) -> bool
{
    for (auto &c : text) {
        if (!_IsAlphabet(c.unicode()))
            return false;
    }
    return true;
}

SIA _IsHexNumber(ushort c) -> bool
{
    return _IsNumber(c) || _InRange<ushort>('a', c, 'f')
                        || _InRange<ushort>('A', c, 'F');
}

SIA _Same(const QString &str, const char *latin1) -> bool
{ return !str.compare(_L(latin1), Qt::CaseInsensitive); }

SIA _Same(const QStringRef &str, const char *latin1) -> bool
{ return !str.compare(_L(latin1), Qt::CaseInsensitive); }

SIA _MidRef(const QStringRef &ref, int from,
                           int n = -1) -> QStringRef
{
    const int count = n < 0 ? ref.size() - from : n;
    return ref.string()->midRef(ref.position() + from, count);
}

SIA _Area(const QSize &size) -> int
{ return size.width()*size.height(); }

SIA _Area(const QSizeF &size) -> qreal
{ return size.width()*size.height(); }

template <class T>
SIA _Change(T &the, const T &one) -> bool
{ if (the != one) {the = one; return true;} return false; }

SIA _ChangeF(double &the, double one) -> bool
{ if (!qFuzzyCompare(the, one)) {the = one; return true;} return false; }

SIA _ChangeZ(double &the, double one) -> bool
{
    if (qFuzzyCompare(one, 1.0))
        one = 1.0;
    if (!qFuzzyCompare(the, one)) {
        the = one;
        return true;
    }
    return false;
}

template<class T>
SIA _C(T& t) -> const T& { return t; }

static const QTime _NullTime(0, 0, 0, 0);

SIA _SecToTime(int sec) -> QTime
{ return _NullTime.addSecs(sec); }

SIA _MSecToTime(qint64 ms) -> QTime
{ return _NullTime.addMSecs(ms); }

SIA _MSecToString(qint64 ms, const QString &fmt = _L("hh:mm:ss")) -> QString
{ return _MSecToTime(ms).toString(fmt); }

SIA _SecToString(int sec, const QString &fmt = _L("hh:mm:ss")) -> QString
{ return _SecToTime(sec).toString(fmt); }

SIA _TimeToMSec(const QTime &time) -> qint64
{ return _NullTime.msecsTo(time); }

SIA _TimeToMSec(int h, int m, int s, int ms = 0) -> qint64
{ return ((h * 60 + m) * 60 + s) * 1000 + ms; }

SIA _Diagonal(double w, double h) -> double
{ return sqrt(w * w + h * h); }

SIA _Diagonal(const QSize &size) -> double
{ return _Diagonal(size.width(), size.height()); }

SIA _Diagonal(const QSizeF &size) -> double
{ return _Diagonal(size.width(), size.height()); }

template <class T1, class T2>
SIA _Ratio(T1 w, T2 h) -> double { return static_cast<double>(w)/h; }

SIA _Ratio(const QSize &s) -> double { return _Ratio(s.width(), s.height()); }

SIA _Ratio(const QSizeF &s) -> double { return _Ratio(s.width(), s.height()); }

SIA _GetOpenFileNames(QWidget *p, const QString &t,
                      const QString &dir, const QString &f) -> QStringList
{ return QFileDialog::getOpenFileNames(p, t, dir, f, 0); }

SIA _GetOpenFileName(QWidget *p, const QString &t,
                     const QString &dir, const QString &f) -> QString
{ return QFileDialog::getOpenFileName(p, t, dir, f, 0); }

SIA _GetSaveFileName(QWidget *p, const QString &t,
                     const QString &dir, const QString &f) -> QString
{ return QFileDialog::getSaveFileName(p, t, dir, f, 0); }

template<class T>
SIA _QmlSingleton(QQmlEngine *, QJSEngine *) -> QObject* { return new T; }

#ifndef __OBJC__

template<class T, typename S = T>
constexpr SIA _Max() -> S { return (S)std::numeric_limits<T>::max(); }

template<class T, typename S = T>
constexpr SIA _Min() -> S { return (S)std::numeric_limits<T>::min(); }

template<class T>
SIA address_cast(const char *address, int base = 10)
-> typename std::enable_if<std::is_pointer<T>::value, T>::type
{
    bool ok = false;
    const quintptr ptr = QString::fromLatin1(address).toULongLong(&ok, base);
    return ok ? (T)(void*)(ptr) : (T)nullptr;
}

template<int N>
constexpr SIA _Aligned(int v) -> int { return v%N ? ((v/N) + 1)*N : v; }

template<class T, typename... Args>
SIA _New(T *&t, Args... args) -> T* { return (t = new T(args...)); }

template<class T, typename... Args>
SIA _Renew(T *&t, Args... args) -> T* {delete t; return (t = new T(args...)); }

template<class T>
SIA _Delete(T *&t) -> void {delete t; t = nullptr; }

template<class Iter, class Test>
SIA _FindIf(Iter begin, Iter end, Test test) -> Iter
{ return std::find_if(begin, end, test); }

template<class List, class Test>
SIA _FindIf(const List &list, Test test) -> typename List::const_iterator
{ return std::find_if(std::begin(list), std::end(list), test); }

template<class List, class Test>
SIA _ContainsIf(const List &l, Test test) -> bool
{ return std::find_if(std::begin(l), std::end(l), test) != std::end(l); }

template<class Iter, class T>
SIA _Find(Iter begin, Iter end, const T &t) -> Iter
{ return std::find(begin, end, t); }

template<class List, class T>
SIA _Find(const List &list, const T &t) -> typename List::const_iterator
{ return std::find(std::begin(list), std::end(list), t); }

template<class List, class T>
SIA _Contains(const List &list, const T &t) -> bool
{ return std::find(std::begin(list), std::end(list), t) != std::end(list); }

#endif

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

static inline auto _SignN(int value, bool sign) -> QString
    { return sign ? _NS(value) : _N(value); }

static inline auto _SignN(double value, bool sign, int n = 1) -> QString
    { return sign ? _NS(value, n) : _N(value, n); }

}

using namespace Pch;
#endif

#endif // STDAFX_HPP
