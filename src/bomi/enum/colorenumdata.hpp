#ifndef COLORENUMDATA_HPP
#define COLORENUMDATA_HPP

struct ColorEnumData {
    ColorEnumData() { }
    ColorEnumData(const QByteArray &o)
    {
        option = o;
        property = display = QString::fromLatin1(o);
    }
    ColorEnumData(const QByteArray &o, const QString &p)
    {
        option = o;
        display = QString::fromLatin1(o);
        property = p;
    }
    ColorEnumData(const QByteArray &o, const QString &p, const QString &d)
    {
        option = o;
        property = p;
        display = d;
    }
    QByteArray option;
    QString property, display;
    auto operator == (const ColorEnumData &rhs) const -> bool
        { return option == rhs.option; }
};

#endif // COLORENUMDATA_HPP

