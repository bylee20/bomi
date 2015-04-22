#ifndef INTERPOLATORPARAMS_HPP
#define INTERPOLATORPARAMS_HPP

#include "enum/interpolator.hpp"

class IntrplParamSet;

class IntrplParamSetWidget : public QWidget {
    Q_OBJECT
public:
    virtual auto setParamSet(const IntrplParamSet &set) -> void = 0;
    virtual auto paramSet() const -> IntrplParamSet = 0;
    virtual auto type() const -> Interpolator = 0;
signals:
    void changed();
protected:
    auto add(QGridLayout *grid, int row, const QString &name, QSlider *&s,
             double min, double max, int dec = 0)
    {
        auto l = new QLabel;
        const double mul = std::pow(10, dec);
        grid->addWidget(new QLabel(name), row, 0);
        grid->addWidget(s = new QSlider(Qt::Horizontal), row, 1);
        grid->addWidget(l, row, 2);
        s->setRange(min * mul, max * mul);
        connect(s, &QSlider::valueChanged, this, [=] (int value)
            { l->setText(_N(value/mul, dec)); emit changed(); });
        l->setText(_N(s->value()/mul, dec));
        if (!dec) {
            s->setPageStep(1);
            s->setSingleStep(1);
        }
    }
};

class IntrplParamSetData {
public:
    virtual ~IntrplParamSetData() = default;
    virtual auto type() const -> Interpolator = 0;
    virtual auto createEditor() const -> IntrplParamSetWidget* = 0;
    virtual auto compare(const IntrplParamSetData *other) const -> bool = 0;
    virtual auto toMpvOption(const QByteArray &prefix) const -> QByteArray = 0;
    virtual auto toJson() const -> QJsonObject = 0;
    virtual auto setFromJson(const QJsonObject &json) -> bool = 0;
    virtual auto copy() const -> IntrplParamSetData* = 0;
    static auto create(Interpolator type) -> IntrplParamSetData*;
protected:
    template<class T> static auto _copy(const T *t) -> T* { return new T(*t); }
    static auto option(const QByteArray &prefix, const QByteArray &name, const QByteArray &value) -> QByteArray
        { QByteArray o; o += ':'; o += prefix; o += '-'; o += name; o += '='; o += value; return o; }
    static auto option(const QByteArray &prefix, const QByteArray &name, int value) -> QByteArray
        { return option(prefix, name, QByteArray::number(value)); }
    static auto option(const QByteArray &prefix, const QByteArray &name, double value) -> QByteArray
        { return option(prefix, name, QByteArray::number(value, 'f', 17)); }
};

class IntrplParamSet {
public:
    enum Preset {
        MitchellNetravali,
        CatmullRom,
        Spline36,
        BSpline,
        EwaLanczosSharp,
        EwaLanczosSoft,
    };
    IntrplParamSet() = default;
    IntrplParamSet(Preset preset);
    IntrplParamSet(Interpolator type): d(IntrplParamSetData::create(type)) { }
    IntrplParamSet(const IntrplParamSet &other): d(other.d->copy()) { }
    IntrplParamSet(IntrplParamSet &&other) { std::swap(d, other.d); }
    ~IntrplParamSet() { delete d; }
    auto operator = (const IntrplParamSet &rhs) -> IntrplParamSet&
        { if (this != &rhs) { delete d; d = rhs.d->copy(); } return *this; }
    auto operator = (IntrplParamSet &&rhs) -> IntrplParamSet&
        { std::swap(d, rhs.d); return *this; }
    auto operator == (const IntrplParamSet &rhs) const -> bool
        { return d->type() == rhs.d->type() && d->compare(rhs.d); }
    auto operator != (const IntrplParamSet &rhs) const -> bool
        { return !operator == (rhs); }
    auto type() const -> Interpolator { return d->type(); }
    auto createEditor() const -> IntrplParamSetWidget* { return d->createEditor(); }
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray { return d->toMpvOption(prefix); }
    auto toJson() const -> QJsonObject { return d->toJson(); }
    auto setFromJson(const QJsonObject &json) -> bool { return d->setFromJson(json); }
    auto data() -> IntrplParamSetData* { return d; }
    auto data() const -> const IntrplParamSetData* { return d; }
private:
    IntrplParamSetData *d = IntrplParamSetData::create(Interpolator::Bilinear);
};


class IntrplParamSetMap {
public:
    IntrplParamSetMap();
    auto operator == (const IntrplParamSetMap &rhs) const -> bool { return m == rhs.m; }
    auto operator != (const IntrplParamSetMap &rhs) const -> bool { return m != rhs.m; }
    auto operator [] (Interpolator type) const -> const IntrplParamSet& { return m[(int)type]; }
    auto set(const IntrplParamSet &set) { return _Change(m[(int)set.type()], set); }
    auto begin() const { return m.begin(); }
    auto end() const { return m.end(); }
    auto cbegin() const { return m.begin(); }
    auto cend() const { return m.end(); }
    auto begin() { return m.begin(); }
    auto end() { return m.end(); }
    auto toJson() const -> QJsonArray;
    auto setFromJson(const QJsonArray &json) -> bool;
    auto size() const -> int { return m.size(); }
private:
    QVector<IntrplParamSet> m;
};

Q_DECLARE_METATYPE(IntrplParamSetMap)

class IntrplDialog : public QDialog {
    Q_OBJECT
public:
    IntrplDialog(QWidget *parent = nullptr);
    ~IntrplDialog();
    auto set(Interpolator intrpl, const IntrplParamSetMap &sets) -> void;
signals:
    void paramsChanged(const IntrplParamSet &params);
private:
    auto setCurrent(const IntrplParamSet &current) -> void;
    struct Data;
    Data *d;
};

#endif // INTERPOLATORPARAMS_HPP
