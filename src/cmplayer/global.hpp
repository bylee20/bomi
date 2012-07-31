#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QSize>
#include <QtCore/QPointF>

#define CONNECT(a, b, c, d) (QObject::connect(a, SIGNAL(b), c, SLOT(d)))

class QDialogButtonBox;		class QDialog;

typedef QLatin1String _LS;
typedef QLatin1Char _LC;

namespace Global {

template<typename T>
static inline const T& const_(T& t) {return t;}

static const QTime __null_time;
static inline QString _U8(const char *utf8) {return QString::fromUtf8(utf8);}

enum class State {Stopped = 1, Playing = 2, Paused = 4, Finished = 8, Opening = 16, Buffering = 32, Error = 64, Preparing = 128};
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

static inline double diagonal(double w, double h) {return sqrt(w*w + h*h);}
static inline double diagonal(const QSize &size) {return diagonal(size.width(), size.height());}
static inline double diagonal(const QSizeF &size) {return diagonal(size.width(), size.height());}

static inline QPointF toPointF(const QSizeF &size) {return QPointF(size.width(), size.height());}

QDialogButtonBox *makeButtonBox(QDialog *dlg);

enum PtrType {PtrObject, PtrArray};
template<PtrType Type> struct __PtrDel {};
template<> struct __PtrDel<PtrObject> {template<typename T>static void free(T *t) {delete t;}};
template<> struct __PtrDel<PtrArray> {template<typename T>static void free(T *t) {delete []t;}};


template <typename T, PtrType Type>
class PtrDel {
public:
	PtrDel(): m_t(nullptr) {}
	PtrDel(T *t): m_t(t) {}
	~PtrDel() {free();}
	PtrDel &operator = (T *t) {free(); m_t = t;}
	T &operator[](int i) {return m_t[i];}
	const T &operator[](int i) const {return m_t[i];}
	inline T& operator *() {return *m_t;}
	inline T* operator ->() {return m_t;}
	inline const T& operator *() const {return *m_t;}
	inline const T* operator ->() const {return m_t;}
	const T*& get() const {return m_t;}
	T*& get() {return m_t;}
	inline bool isNull() const {return m_t == nullptr;}
private:
	T *m_t;
	PtrDel(const PtrDel &);
	PtrDel &operator = (const PtrDel &);
	inline void free() {__PtrDel<Type>::free(m_t);}
};

template<typename T> class AutoObject : public PtrDel<T, PtrObject> {};
template<typename T> class AutoArray : public PtrDel<T, PtrArray> {};
}

using namespace Global;

#endif // GLOBAL_HPP
