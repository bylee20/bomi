#ifndef STDAFX_HPP
#define STDAFX_HPP

#ifdef __cplusplus

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQuick>
#include <QtOpenGL>
#include <set>
#include <qmath.h>

#ifdef Q_WS_MAC
#define Q_OS_MAC
#endif
#ifdef Q_WS_X11
#define Q_OS_X11
#endif

extern "C" void *fast_memcpy(void * to, const void * from, size_t len);

namespace Pch {
static inline QLatin1String _L(const char *str) {return QLatin1String(str);}
static inline QLatin1Char _L(char c) {return QLatin1Char(c);}
static inline QString _u(const char *utf8, int len = -1) {return QString::fromUtf8(utf8, len);}
static inline QString _n(int n, int base = 10, int width = 0, const QChar &c = _L(' ')) {return QString("%1").arg(n, width, base, c);}
static inline QString _n(quint32 n, int base = 10) {return QString::number(n, base);}
static inline QString _n(quint64 n, int base = 10) {return QString::number(n, base);}
static inline QString _n(double n, int dec = 1) {return QString::number(n, 'f', dec);}
static inline QString _n(double n, int dec, int width, const QChar &c = QChar(QChar::Nbsp)) {return QString("%1").arg(n, width, 'f', dec, c);}
static inline QString chopped(const QString &str, int n) {QString ret = str; ret.chop(n); return ret;}
template<typename T> static inline const T& _c(T& t) {return t;}
static const QTime __null_time(0, 0, 0, 0);
static inline QTime secToTime(int sec) {return __null_time.addSecs(sec);}
static inline QTime msecToTime(qint64 ms) {return __null_time.addMSecs(ms);}
static inline QString msecToString(qint64 ms, const QString &fmt = _L("hh:mm:ss")) {return msecToTime(ms).toString(fmt);}
static inline QString secToString(int s, const QString &fmt = _L("hh:mm:ss")) {return secToTime(s).toString(fmt);}
static inline qint64 timeToMSec(const QTime &time) {return __null_time.msecsTo(time);}
static inline qint64 timeToMSec(int h, int m, int s, int ms = 0) {return ((h*60 + m)*60 + s)*1000 + ms;}
static inline double diagonal(double w, double h) {return sqrt(w*w + h*h);}
static inline double diagonal(const QSize &size) {return diagonal(size.width(), size.height());}
static inline double diagonal(const QSizeF &size) {return diagonal(size.width(), size.height());}
static inline QStringList getOpenFileNames(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getOpenFileNames(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}
static inline QString getOpenFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getOpenFileName(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}
static inline QString getSaveFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getSaveFileName(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}
}

using namespace Pch;
#endif

#endif // STDAFX_HPP
