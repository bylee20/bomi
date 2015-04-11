#ifndef FORMATOBJECT_HPP
#define FORMATOBJECT_HPP

#include <QQuickItem>

class FormatObject : public QObject {
    Q_OBJECT
public:
    FormatObject(QObject *parent = nullptr);
    ~FormatObject();
public slots:
    QString time(int msec, bool point = false, bool hour = false);
    QString time(int msec, const QString &format);
    QString textNA(const QString &text, const QString &na = "--"_a, const QString &no = "no"_a);
    QString integerNA(int n, int min = 1, const QString &na = "--"_a);
    QString fixedNA(qreal n, int points, qreal min = 1, const QString &na = "--"_a);
    QString integerNA(int n, const QString &suffix, int min = 1, const QString &na = "--"_a);
    QString fixedNA(qreal n, int points, const QString &suffix, qreal min = 1, const QString &na = "--"_a);
    QString sizeNA(int w, int h, const QString &na = "--"_a);
    QString sizeNA(qreal w, qreal h, int points, const QString &na = "--"_a);
    QString sizeNA(const QSize &s, const QString &na = "--"_a);
    QString sizeNA(const QSizeF &s, int points, const QString &na = "--"_a);
    QString listNumber(int n) { return integerNA(n, 1, "-"_a); }
    QString listNumber(int n, int len) { return listNumber(n) % "/"_a % listNumber(len); }
};

#endif // FORMATOBJECT_HPP
