#ifndef STDAFX_HPP
#define STDAFX_HPP

#ifdef __cplusplus

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQuick>
#include <QtSql>
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
#endif

namespace Pch {
static inline QLatin1String _L(const char *str) {return QLatin1String(str);}
static inline QLatin1Char _L(char c) {return QLatin1Char(c);}
static inline QString _U(const char *utf8, int len = -1) {return QString::fromUtf8(utf8, len);}
static inline QString _N(int n, int base = 10, int width = 0, const QChar &c = _L(' ')) {return QString("%1").arg(n, width, base, c);}
static inline QString _N(quint32 n, int base = 10) {return QString::number(n, base);}
static inline QString _N(quint64 n, int base = 10) {return QString::number(n, base);}
static inline QString _N(double n, int dec = 1) {return QString::number(n, 'f', dec);}
static inline QString _N(double n, int dec, int width, const QChar &c = QChar(QChar::Nbsp)) {return QString("%1").arg(n, width, 'f', dec, c);}
static inline QString _Chopped(const QString &str, int n) {QString ret = str; ret.chop(n); return ret;}
// number with sign
template<typename N>
static inline QString _SignSymbol(N n) { return n < 0 ? _L("-") : (n > 0 ? _L("+") : _U("±")); }
static inline QString _NS(int n) { return _SignSymbol(n) % _N(qAbs(n)); }
static inline QString _NS(double n, int dec = 1) { return _SignSymbol(n) % _N(qAbs(n), dec); }

template<typename T>
static inline bool _InRange(const T &min, const T &val, const T &max) {return min <= val && val <= max;}
static inline bool _IsNumber(ushort c) {return _InRange<ushort>('0', c, '9');}
static inline bool _IsAlphabet(ushort c) {return _InRange<ushort>('a', c, 'z') || _InRange<ushort>('A', c, 'Z');}
static inline bool _IsHexNumber(ushort c) {return _IsNumber(c) || _InRange<ushort>('a', c, 'f') || _InRange<ushort>('A', c, 'F');}
static inline bool _Same(const QString &str, const char *latin1) {return !str.compare(_L(latin1), Qt::CaseInsensitive);}
static inline bool _Same(const QStringRef &str, const char *latin1) {return !str.compare(_L(latin1), Qt::CaseInsensitive);}
static inline QStringRef _MidRef(const QStringRef &ref, int from, int n = -1) {return ref.string()->midRef(ref.position() + from, n < 0 ? ref.size() - from : n);}
static inline int _Area(const QSize &size) {return size.width()*size.height();}
static inline qreal _Area(const QSizeF &size) { return size.width()*size.height(); }
template <typename T>
static inline bool _Change(T &the, const T &one) {if (the != one) {the = one; return true;} return false;}
static inline bool _ChangeF(double &the, double one) {if (!qFuzzyCompare(the, one)) {the = one; return true;} return false;}
static inline bool _ChangeZ(double &the, double one) {if (qFuzzyCompare(one, 1.0)) {one = 1.0;} if (!qFuzzyCompare(the, one)) {the = one; return true;} return false;}
template<typename T> static inline const T& _C(T& t) {return t;}
static const QTime _NullTime(0, 0, 0, 0);
static inline QTime _SecToTime(int sec) {return _NullTime.addSecs(sec);}
static inline QTime _MSecToTime(qint64 ms) {return _NullTime.addMSecs(ms);}
static inline QString _MSecToString(qint64 ms, const QString &fmt = _L("hh:mm:ss")) {return _MSecToTime(ms).toString(fmt);}
static inline QString _SecToString(int s, const QString &fmt = _L("hh:mm:ss")) {return _SecToTime(s).toString(fmt);}
static inline qint64 _TimeToMSec(const QTime &time) {return _NullTime.msecsTo(time);}
static inline qint64 _TimeToMSec(int h, int m, int s, int ms = 0) {return ((h*60 + m)*60 + s)*1000 + ms;}
static inline double _Diagonal(double w, double h) {return sqrt(w*w + h*h);}
static inline double _Diagonal(const QSize &size) {return _Diagonal(size.width(), size.height());}
static inline double _Diagonal(const QSizeF &size) {return _Diagonal(size.width(), size.height());}
template <typename T1, typename T2>
static inline double _Ratio(T1 w, T2 h) {return static_cast<double>(w)/static_cast<double>(h);}
static inline double _Ratio(const QSize &size) {return _Ratio(size.width(), size.height());}
static inline double _Ratio(const QSizeF &size) {return _Ratio(size.width(), size.height());}
static inline QStringList _GetOpenFileNames(QWidget *p, const QString &t, const QString &dir, const QString &f) {
    return QFileDialog::getOpenFileNames(p, t, dir, f, 0);
}
static inline QString _GetOpenFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
    return QFileDialog::getOpenFileName(p, t, dir, f, 0);
}
static inline QString _GetSaveFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
    return QFileDialog::getSaveFileName(p, t, dir, f, 0);
}

#ifndef __OBJC__
template<typename T, typename S = T> constexpr static inline S _Max() { return (S)std::numeric_limits<T>::max(); }
template<typename T, typename S = T> constexpr static inline S _Min() { return (S)std::numeric_limits<T>::min(); }

template<typename T> static typename std::enable_if<std::is_pointer<T>::value, T>::type address_cast(const char *address, int base = 10) {
    bool ok = false;
    const quintptr ptr = QString::fromLatin1(address).toULongLong(&ok, base);
    return ok ? (T)(void*)(ptr) : (T)nullptr;
}

template<int N> constexpr static int _Aligned(int v) { return v%N ? ((v/N) + 1)*N : v; }
template<typename T, typename... Args> T *  _New(T *&t, Args... args) { return (t = new T(args...)); }
template<typename T, typename... Args> T *  _Renew(T *&t, Args... args) {delete t; return (t = new T(args...)); }
template<typename T>                   void _Delete(T *&t) {delete t; t = nullptr; }
template<typename Iter, typename Test>
Iter _FindIf(Iter begin, Iter end, Test test) { return std::find_if(begin, end, test); }
template<typename List, typename Test>
typename List::const_iterator _FindIf(const List &list, Test test) { return std::find_if(std::begin(list), std::end(list), test); }
template<typename List, typename Test>
bool _ContainsIf(const List &list, Test test) { return std::find_if(std::begin(list), std::end(list), test) != std::end(list); }

template<typename Iter, typename T>
Iter _Find(Iter begin, Iter end, const T &t) { return std::find(begin, end, t); }
template<typename List, typename T>
typename List::const_iterator _Find(const List &list, const T &t) { return std::find(std::begin(list), std::end(list), t); }
template<typename List, typename T>
bool _Contains(const List &list, const T &t) { return std::find(std::begin(list), std::end(list), t) != std::end(list); }

#endif

static inline quint64 _SystemTime() { struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }

template<typename T> bool _Expand(T &t, int size, double extra = 1.2) { if (t.size() < size) {t.resize(size*extra); return true;} return false; }

}

using namespace Pch;
#endif

#endif // STDAFX_HPP
