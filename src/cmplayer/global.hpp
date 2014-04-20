#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "stdafx.hpp"

#define CONNECT(a, b, c, d) (QObject::connect(a, SIGNAL(b), c, SLOT(d)))

namespace Global {

template<typename T>
struct Range {
    Range() {}
    Range(const T &min, const T &max): min(min), max(max) {}
    bool contains(const T &t) {return min <= t && t <= max;}
    bool isValid() const {return min <= max;}
    T difference() const {return max - min;}
    T min = 0, max = 0;
};
typedef Range<double> RangeF;
typedef Range<int> RangeI;

enum StreamType {UnknownStream = 0, VideoStream, AudioStream, SubPicStream};
enum MediaMetaData {LanguageCode};

static inline QString toString(int value, bool sign) {
    if (!sign || value < 0) return QString::number(value);
    return (value > 0 ? _L("+") : _U("±")) += QString::number(value);
}

static inline QString toString(double value, bool sign, int n = 1) {
    if (n <= 0)
        return toString(qRound(value), sign);
    QString ret;
    if (sign && value >= 0)
        ret = (value > 0 ? _L("+") : _U("±"));
    QByteArray fmt("%.");    fmt.reserve(10);
    fmt.append(QByteArray::number(n)).append("f");
    return ret += QString().sprintf(fmt.data(), value);
}

static inline QString toString(const QSize &size) {
    QString ret = QString::number(size.width()); ret.reserve(ret.size()*2 + 5);
    ret += QString::fromUtf8("\303\227"); ret += QString::number(size.height());
    return ret;
}

static inline QPointF toPointF(const QSizeF &size) {return QPointF(size.width(), size.height());}

QByteArray _Uncompress(const QByteArray &data);
}

using namespace Global;

#endif // GLOBAL_HPP
