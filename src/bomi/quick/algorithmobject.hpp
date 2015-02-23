#ifndef ALGORITHMOBJECT_HPP
#define ALGORITHMOBJECT_HPP

#include <cmath>

class AlgorithmObject : public QObject {
    Q_OBJECT
public:
    AlgorithmObject(QObject *parent = nullptr);
    ~AlgorithmObject();
public slots:
    int trunc(qreal x) const { return std::trunc(x); }
    int ceil(qreal x) const { return std::ceil(x); }
    int floor(qreal x) const { return std::floor(x); }
    int round(qreal x) const { return std::round(x); }
    qreal trunc(qreal x, int point) const
        { const qreal mul = std::pow(10, point); return trunc(x * mul) / mul; }
    qreal ceil(qreal x, int point) const
        { const qreal mul = std::pow(10, point); return ceil(x * mul) / mul; }
    qreal floor(qreal x, int point) const
        { const qreal mul = std::pow(10, point); return floor(x * mul) / mul; }
    qreal round(qreal x, int point) const
        { const qreal mul = std::pow(10, point); return round(x * mul) / mul; }

    int clamp(int v, int min, int max) { return v < min ? min : v > max ? max : v; }
    qreal clamp(qreal v, qreal min, qreal max) { return v < min ? min : v > max ? max : v; }
};

#endif // ALGORITHMOBJECT_HPP
