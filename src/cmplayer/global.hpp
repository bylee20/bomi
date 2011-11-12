#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QSize>

#define CONNECT(a, b, c, d) (QObject::connect(a, SIGNAL(b), c, SLOT(d)))

class QDialogButtonBox;		class QDialog;

typedef QLatin1String _LS;
typedef QLatin1Char _LC;

namespace Global {

template<typename T>
static inline const T& const_(T& t) {return t;}

static const QTime __null_time;
static inline QString _U8(const char *utf8) {return QString::fromUtf8(utf8);}

enum MediaState {StoppedState = 0, PlayingState, PausedState, FinishedState};
enum MediaStatus {NoMediaStatus = 0, EosStatus, BufferedStatus};
enum StreamType {UnknownStream = 0, VideoStream, AudioStream, SubPicStream};
enum MediaMetaData {LanguageCode};

static inline QTime secToTime(int sec) {return __null_time.addSecs(sec);}
static inline QTime msecToTime(qint64 ms) {return __null_time.addMSecs(ms);}
static inline QString msecToString(qint64 ms, const QString &fmt = _LS("hh:mm:ss")) {return msecToTime(ms).toString(fmt);}
static inline QString secToString(int s, const QString &fmt = _LS("hh:mm:ss")) {return secToTime(s).toString(fmt);}
static inline qint64 timeToMSec(const QTime &time) {return __null_time.msecsTo(time);}
static inline qint64 timeToMSec(int h, int m, int s, int ms = 0) {return ((h*60 + m)*60 + s)*1000 + ms;}

QStringList getOpenFileNames(QWidget *p, const QString &t, const QString &dir, const QString &f);
QString getOpenFileName(QWidget *p, const QString &t, const QString &dir, const QString &f);
QString getSaveFileName(QWidget *p, const QString &t, const QString &dir, const QString &f);

static inline QString toString(int value, bool sign) {
	if (!sign || value < 0) return QString::number(value);
	return (value > 0 ? _LS("+") : _U8("±")) += QString::number(value);
}

static inline QString toString(double value, bool sign, int n = 1) {
	if (n <= 0)
		return toString(qRound(value), sign);
	QString ret;
	if (sign && value >= 0)
		ret = (value > 0 ? _LS("+") : _U8("±"));
	QByteArray fmt("%.");	fmt.reserve(10);
	fmt.append(QByteArray::number(n)).append("f");
	return ret += QString().sprintf(fmt.data(), value);
}

static inline QString toString(const QSize &size) {
	QString ret = QString::number(size.width()); ret.reserve(ret.size()*2 + 5);
	ret += QString::fromUtf8("\303\227"); ret += QString::number(size.height());
	return ret;
}

QDialogButtonBox *makeButtonBox(QDialog *dlg);



}

using namespace Global;

#endif // GLOBAL_HPP
