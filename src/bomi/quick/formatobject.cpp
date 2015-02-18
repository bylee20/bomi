#include "formatobject.hpp"
#include "player/avinfoobject.hpp"
#include <QQmlProperty>

FormatObject::FormatObject(QObject *parent)
    : QObject(parent)
{

}

FormatObject::~FormatObject()
{

}

QString FormatObject::time(int msec, bool point)
{
    QString sign;
    if (msec < 0) {
        sign += '-'_q;
        msec = -msec;
    }
    int hours = msec / 3600000;
    msec %= 3600000;
    int mins = msec / 60000;
    msec %= 60000;
    int secs = msec / 1000;
    msec %= 1000;

    return sign % _N(hours)
            % (mins < 10 ? ":0"_a : ":"_a) % _N(mins)
            % (secs < 10 ? ":0"_a : ":"_a) % _N(secs)
            % (point ? QString('.'_q % _N(msec, 10, 3, '0'_q)) : u""_q);
}

QString FormatObject::time(int msec, const QString &format)
{
    return _MSecToString(msec, format);
}

QString FormatObject::textNA(const QString &text, const QString &na, const QString &no)
{
    if (text.isEmpty() || !text.compare(no, Qt::CaseInsensitive))
        return na;
    return text;
}

QString FormatObject::integerNA(int n, int min, const QString &na)
{
    return (n < min) ? na : _N(n);
}

QString FormatObject::fixedNA(qreal n, int points, qreal min, const QString &na)
{
    if (n < min)
        return na;
    if (points < 0)
        return QString::number(n, 'g', 6);
    return QString::number(n, 'f', points);
}

QString FormatObject::sizeNA(int w, int h, const QString &na)
{
    return integerNA(w, 1, na) % 'x'_q % integerNA(h, 1, na);
}

QString FormatObject::sizeNA(qreal w, qreal h, int points, const QString &na)
{
    return fixedNA(w, points, 0.1, na) % 'x'_q % fixedNA(h, points, 0.1, na);
}

QString FormatObject::sizeNA(const QSize &s, const QString &na)
{
    return sizeNA(s.width(), s.height(), na);
}

QString FormatObject::sizeNA(const QSizeF &s, int points, const QString &na)
{
    return sizeNA(s.width(), s.height(), points, na);
}

QString FormatObject::sizeNA(const QQuickItem *item, const QString &na)
{
    return sizeNA(item->width(), item->height(), -1, na);
}

QString FormatObject::sizeNA(const QObject *o, const QString &na)
{
    auto size = o->property("size");
    if (size.userType() == QMetaType::QSize)
        return sizeNA(size.toSize(), na);
    if (size.userType() == QMetaType::QSizeF)
        return sizeNA(size.toSizeF(), 0, na);
    auto w = o->property("width");
    auto h = o->property("height");
    if (_IsOneOf(w.userType(), QMetaType::Float, QMetaType::Double))
        return sizeNA(w.toDouble(), h.toDouble(), -1, na);
    return sizeNA(w.toInt(), h.toInt(), na);
}

QString FormatObject::trackInfo(QObject *o)
{
    auto av = qobject_cast<AvCommonObject*>(o);
    if (!av)
        return listNumber(0, 0);
    QQmlProperty length(av, "tracks.length"_a);
    return listNumber(av->track()->number(), length.read().toInt());
}
