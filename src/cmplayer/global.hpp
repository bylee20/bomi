#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "stdafx.hpp"

namespace Global {

template<class T>
struct Range {
    Range() {}
    Range(const T &min, const T &max): min(min), max(max) {}
    auto contains(const T &t) -> bool {return min <= t && t <= max;}
    auto isValid() const -> bool {return min <= max;}
    auto difference() const -> T {return max - min;}
    T min = 0, max = 0;
};
typedef Range<double> RangeF;
typedef Range<int> RangeI;

enum StreamType {UnknownStream = 0, VideoStream, AudioStream, SubPicStream};
enum MediaMetaData {LanguageCode};

static inline auto toString(int value, bool sign) -> QString {
    if (!sign || value < 0) return QString::number(value);
    return (value > 0 ? _L("+") : _U("±")) += QString::number(value);
}

static inline auto toString(double value, bool sign, int n = 1) -> QString {
    if (n <= 0)
        return toString(qRound(value), sign);
    QString ret;
    if (sign && value >= 0)
        ret = (value > 0 ? _L("+") : _U("±"));
    QByteArray fmt("%.");    fmt.reserve(10);
    fmt.append(QByteArray::number(n)).append("f");
    return ret += QString().sprintf(fmt.data(), value);
}

static inline auto toString(const QSize &size) -> QString {
    QString ret = QString::number(size.width()); ret.reserve(ret.size()*2 + 5);
    ret += QString::fromUtf8("\303\227"); ret += QString::number(size.height());
    return ret;
}

static inline auto toPointF(const QSizeF &size) -> QPointF {return QPointF(size.width(), size.height());}

auto _Uncompress(const QByteArray &data) -> QByteArray;
}

using namespace Global;

#endif // GLOBAL_HPP
