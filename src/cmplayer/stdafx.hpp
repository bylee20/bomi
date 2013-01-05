#ifndef STDAFX_HPP
#define STDAFX_HPP

#include <sigar.h>

#ifdef __cplusplus

#include <QtCore>
#include <QtGui>
#include <QtOpenGL>
#include <set>
#include <qmath.h>

namespace Pch {
static inline QLatin1String _L(const char *str) {return QLatin1String(str);}
static inline QLatin1Char _L(char c) {return QLatin1Char(c);}
static inline QString _u(const char *utf8) {return QString::fromUtf8(utf8);}
template<typename T> static inline const T& _c(T& t) {return t;}
static const QTime __null_time;
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
