#include "interpolatorparams.hpp"
#include "widget/enumcombobox.hpp"
#include "misc/json.hpp"
#include "tmp/algorithm.hpp"

IntrplParamSetMap::IntrplParamSetMap()
{
    static const auto def = [] () {
        QVector<IntrplParamSet> sets;
        for (auto &item : InterpolatorInfo::items())
            sets.push_back(Interpolator(item.value));
        return sets;
    }();
    m = def;
}

auto IntrplParamSetMap::toJson() const -> QJsonArray
{
    QJsonArray json;
    for (auto &s : m)
        json.push_back(s.toJson());
    return json;
}

auto IntrplParamSetMap::setFromJson(const QJsonArray &json) -> bool
{
    IntrplParamSetMap map;
    if (json.size() != map.size())
        return false;
    for (int i = 0; i < json.size(); ++i) {
        if (!map.m[i].setFromJson(json[i].toObject()))
            return false;
    }
    m = map.m;
    return true;
}

class BilinearParamSet : public IntrplParamSetData {
    struct Widget : public IntrplParamSetWidget {
        auto setParamSet(const IntrplParamSet &) -> void final { }
        auto paramSet() const -> IntrplParamSet final
            { return IntrplParamSet(type()); }
        auto type() const -> Interpolator final { return Interpolator::Bilinear; }
    };
public:
    auto type() const -> Interpolator final { return Interpolator::Bilinear; }
    auto createEditor() const -> IntrplParamSetWidget* final { return new Widget; }
    auto compare(const IntrplParamSetData *other) const -> bool final { return other->type() == type(); }
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final { Q_UNUSED(prefix); return _EnumData(type()); }
    auto toJson() const -> QJsonObject final { QJsonObject json; json[u"type"_q] = _ToJson(type()); return json; }
    auto setFromJson(const QJsonObject &json) -> bool { return _FromJson<Interpolator>(json[u"type"_q]) == type(); }
    auto copy() const -> IntrplParamSetData* { return _copy(this); }
};

class BicubicParamSet : public IntrplParamSetData {
    struct Widget : public IntrplParamSetWidget {
        Widget()
        {
            auto grid = new QGridLayout(this);
            add(grid, 0, IntrplParamSetWidget::tr("B-parameter"), m_b, 0, 1, 2);
            add(grid, 1, IntrplParamSetWidget::tr("C-parameter"), m_c, 0, 1, 2);
            add(grid, 2, IntrplParamSetWidget::tr("Anti-ringing"), m_ar, 0, 1, 2);
        }
        auto setParamSet(const IntrplParamSet &set) -> void
        {
            Q_ASSERT(set.type() == Interpolator::Bicubic);
            auto p = static_cast<const BicubicParamSet*>(set.data());
            m_b->setValue(p->b * 100 + 0.5);
            m_c->setValue(p->c * 100 + 0.5);
            m_ar->setValue(p->antiring * 100 + 0.5);
        }
        auto paramSet() const -> IntrplParamSet
        {
            IntrplParamSet set(Interpolator::Bicubic);
            auto p = static_cast<BicubicParamSet*>(set.data());
            p->b = m_b->value() * 1e-2;
            p->c = m_c->value() * 1e-2;
            p->antiring = m_ar->value() * 1e-2;
            return set;
        }
        auto type() const -> Interpolator final { return Interpolator::Bicubic; }
    private:
        QSlider *m_b, *m_c, *m_ar;
    };
public:
    auto type() const -> Interpolator final { return Interpolator::Bicubic; }
    auto createEditor() const -> IntrplParamSetWidget* final { return new Widget; }
    auto compare(const IntrplParamSetData *other) const -> bool final {
        if (other->type() != type())
            return false;
        auto p = static_cast<const BicubicParamSet*>(other);
        return p->b == b && p->c == c && p->antiring == antiring;
    }
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final
    {
        QByteArray opts;
        opts += _EnumData(type());
        opts += option(prefix, "param1"_b, b);
        opts += option(prefix, "param2"_b, c);
        return opts;
    }
    auto toJson() const -> QJsonObject final
    {
        QJsonObject json;
        json[u"type"_q] = _ToJson(type());
        json[u"b"_q] = b;
        json[u"c"_q] = c;
        json[u"antiring"_q] = antiring;
        return json;
    }
    auto setFromJson(const QJsonObject &json) -> bool
    {
        if (_FromJson<Interpolator>(json[u"type"_q]) != type())
            return false;
        b = json[u"b"_q].toDouble();
        c = json[u"c"_q].toDouble();
        antiring = json[u"antiring"_q].toDouble();
        return true;
    }
    auto copy() const -> IntrplParamSetData* final { return _copy(this); }
    double b = 0.0, c = 0.5; double antiring = 0.0;
};

template<class T>
class SimpleRadiusParamSet : public IntrplParamSetData {
    struct Widget : public IntrplParamSetWidget {
        Widget()
        {
            auto grid = new QGridLayout(this);
            add(grid, 0, IntrplParamSetWidget::tr("Radius"), m_radius, 2, 4);
            add(grid, 1, IntrplParamSetWidget::tr("Anti-ringing"), m_antiring, 0, 1, 2);
        }
        auto setParamSet(const IntrplParamSet &set) -> void
        {
            auto p = static_cast<const SimpleRadiusParamSet*>(set.data());
            m_radius->setValue(p->radius);
            m_antiring->setValue(p->antiring * 100 + 0.5);
        }
        auto paramSet() const -> IntrplParamSet
        {
            IntrplParamSet set(T::_type);
            auto p = static_cast<T*>(set.data());
            p->radius = m_radius->value();
            p->antiring = m_antiring->value() * 1e-2;
            return set;
        }
        auto type() const -> Interpolator final { return T::_type; }
    private:
        QSlider *m_radius, *m_antiring;
    };
public:
    auto type() const -> Interpolator final { return T::_type; }
    auto createEditor() const -> IntrplParamSetWidget* final { return new Widget; }
    auto compare(const IntrplParamSetData *other) const -> bool final {
        if (other->type() != type())
            return false;
        auto p = static_cast<const SimpleRadiusParamSet*>(other);
        return p->radius == radius && p->antiring == antiring;
    }
    auto toJson() const -> QJsonObject final
    {
        QJsonObject json;
        json[u"type"_q] = _ToJson(type());
        json[u"radius"_q] = radius;
        json[u"antiring"_q] = antiring;
        return json;
    }
    auto setFromJson(const QJsonObject &json) -> bool
    {
        if (_FromJson<Interpolator>(json[u"type"_q]) != type())
            return false;
        radius = json[u"radius"_q].toInt();
        antiring = json[u"antiring"_q].toDouble();
        return true;
    }
    int radius = 3; double antiring = 0.0;
};

class SplineParamSet : public SimpleRadiusParamSet<SplineParamSet> {
public:
    static constexpr auto _type = Interpolator::Spline;
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final
    {
        QByteArray opts;
        opts += _EnumData(type());
        if (radius == 2)
            opts += "16"_b;
        else if (radius == 4)
            opts += "64"_b;
        else
            opts += "36"_b;
        opts += option(prefix, "antiring"_b, antiring);
        return opts;
    }
    auto copy() const -> IntrplParamSetData* { return _copy(this); }
};

class LanczosParamSet : public SimpleRadiusParamSet<LanczosParamSet> {
public:
    static constexpr auto _type = Interpolator::Lanczos;
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final
    {
        QByteArray opts;
        opts += _EnumData(type());
        opts += option(prefix, "radius"_b, radius);
        opts += option(prefix, "antiring"_b, antiring);
        return opts;
    }
    auto copy() const -> IntrplParamSetData* { return _copy(this); }
};

class EwaLanczosParamSet : public IntrplParamSetData {
    struct Widget : public IntrplParamSetWidget {
        Widget()
        {
            auto grid = new QGridLayout(this);
            grid->addWidget(new QLabel(IntrplParamSetWidget::tr("Radius")), 0, 0);
            grid->addWidget(m_radius = new QDoubleSpinBox, 0, 1, 1, -1);
            m_radius->setDecimals(16);
            m_radius->setRange(0.5, 16);
            m_radius->setSingleStep(0.1);
            connect(SIGNAL_VT(m_radius, valueChanged, double),
                    this, &IntrplParamSetWidget::changed);
            grid->addWidget(new QLabel(IntrplParamSetWidget::tr("Blur")), 1, 0);
            grid->addWidget(m_blur = new QDoubleSpinBox, 1, 1, 1, -1);
            m_blur->setDecimals(16);
            m_blur->setRange(0.0, 1.2);
            m_blur->setSingleStep(0.1);
            m_blur->setSpecialValueText(IntrplParamSetWidget::tr("Default value"));
            connect(SIGNAL_VT(m_blur, valueChanged, double),
                    this, &IntrplParamSetWidget::changed);
            add(grid, 2, IntrplParamSetWidget::tr("Anti-ringing"), m_antiring, 0, 1, 2);
        }
        auto setParamSet(const IntrplParamSet &set) -> void
        {
            auto p = static_cast<const EwaLanczosParamSet*>(set.data());
            m_radius->setValue(p->radius);
            m_antiring->setValue(p->antiring * 100 + 0.5);
            m_blur->setValue(p->blur);
        }
        auto paramSet() const -> IntrplParamSet
        {
            IntrplParamSet set(Interpolator::EwaLanczos);
            auto p = static_cast<EwaLanczosParamSet*>(set.data());
            p->radius = m_radius->value();
            p->antiring = m_antiring->value() * 1e-2;
            p->blur = m_blur->value();
            return set;
        }
        auto type() const -> Interpolator final { return Interpolator::EwaLanczos; }
    private:
        QDoubleSpinBox *m_radius, *m_blur;
        QSlider *m_antiring;
    };
public:
    auto type() const -> Interpolator final { return Interpolator::EwaLanczos; }
    auto createEditor() const -> IntrplParamSetWidget* final { return new Widget; }
    auto compare(const IntrplParamSetData *other) const -> bool final {
        if (other->type() != type())
            return false;
        auto p = static_cast<const EwaLanczosParamSet*>(other);
        return p->radius == radius && p->antiring == antiring && p->blur == blur;
    }
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final
    {
        QByteArray opts;
        opts += _EnumData(type());
        opts += option(prefix, "radius"_b, radius);
        opts += option(prefix, "antiring"_b, antiring);
        if (blur > 0.4)
            opts += option(prefix, "blur"_b, blur);
        return opts;
    }
    auto toJson() const -> QJsonObject final
    {
        QJsonObject json;
        json[u"type"_q] = _ToJson(type());
        json[u"radius"_q] = radius;
        json[u"antiring"_q] = antiring;
        json[u"blur"_q] = blur;
        return json;
    }
    auto setFromJson(const QJsonObject &json) -> bool
    {
        if (_FromJson<Interpolator>(json[u"type"_q]) != type())
            return false;
        radius = json[u"radius"_q].toDouble();
        antiring = json[u"antiring"_q].toDouble();
        blur = json[u"blur"_q].toDouble();
        return true;
    }
    auto copy() const -> IntrplParamSetData* { return _copy(this); }
    double radius = 3, antiring = 0.0, blur = 0.0;
};

class SharpenParamSet : public IntrplParamSetData {
    struct Widget : public IntrplParamSetWidget {
        Widget()
        {
            auto grid = new QGridLayout(this);
            grid->addWidget(new QLabel(IntrplParamSetWidget::tr("Kernel size")), 0, 0);
            grid->addWidget(m_kernel = new QComboBox, 0, 1, 1, -1);
            add(grid, 1, IntrplParamSetWidget::tr("Sharpness"), m_sharpness, 0, 10, 2);
            m_kernel->addItem(u"3x3"_q, 3);
            m_kernel->addItem(u"3x3"_q, 5);
            connect(SIGNAL_VT(m_kernel, currentIndexChanged, int),
                    this, &IntrplParamSetWidget::changed);
        }
        auto setParamSet(const IntrplParamSet &set) -> void
        {
            auto p = static_cast<const SharpenParamSet*>(set.data());
            m_kernel->setCurrentIndex(m_kernel->findData(p->kernel));
            m_sharpness->setValue(p->sharpeness * 100 + 0.5);
        }
        auto paramSet() const -> IntrplParamSet
        {
            IntrplParamSet set(Interpolator::Sharpen);
            auto p = static_cast<SharpenParamSet*>(set.data());
            p->kernel = m_kernel->currentData().toInt();
            p->sharpeness = m_sharpness->value() * 1e-2;
            return set;
        }
        auto type() const -> Interpolator final { return Interpolator::Sharpen; }
    private:
        QComboBox *m_kernel;
        QSlider *m_sharpness;
    };
public:
    auto type() const -> Interpolator final { return Interpolator::Sharpen; }
    auto createEditor() const -> IntrplParamSetWidget* final { return new Widget; }
    auto compare(const IntrplParamSetData *other) const -> bool final {
        if (other->type() != type())
            return false;
        auto p = static_cast<const SharpenParamSet*>(other);
        return p->kernel == kernel && p->sharpeness == sharpeness;
    }
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray final
    {
        QByteArray opts;
        opts += _EnumData(type());
        opts += kernel == 5 ? "5"_b : "3"_b;
        opts += option(prefix, "param1"_b, sharpeness);
        return opts;
    }
    auto toJson() const -> QJsonObject final
    {
        QJsonObject json;
        json[u"type"_q] = _ToJson(type());
        json[u"kernel"_q] = kernel;
        json[u"sharpeness"_q] = sharpeness;
        return json;
    }
    auto setFromJson(const QJsonObject &json) -> bool
    {
        if (_FromJson<Interpolator>(json[u"type"_q]) != type())
            return false;
        kernel = json[u"kernel"_q].toInt();
        sharpeness = json[u"sharpeness"_q].toDouble();
        return true;
    }
    auto copy() const -> IntrplParamSetData* { return _copy(this); }
    int kernel = 3; double sharpeness = 0.5;
};

auto IntrplParamSetData::create(Interpolator type) -> IntrplParamSetData*
{
    switch (type) {
    case Interpolator::Bilinear:
        return new BilinearParamSet;
    case Interpolator::Bicubic:
        return new BicubicParamSet;
    case Interpolator::Spline:
        return new SplineParamSet;
    case Interpolator::Lanczos:
        return new LanczosParamSet;
    case Interpolator::EwaLanczos:
        return new EwaLanczosParamSet;
    case Interpolator::Sharpen:
        return new SharpenParamSet;
    }
    abort();
    return nullptr;
}

IntrplParamSet::IntrplParamSet(Preset preset)
    : d(nullptr)
{
    switch (preset) {
    case BSpline:
    case CatmullRom:
    case MitchellNetravali: {
        auto p = new BicubicParamSet;
        if (preset == BSpline) { p->b = p->c = 0.5; }
        else if (preset == CatmullRom) { p->b = 0; p->c = 0.5; }
        else if (preset == MitchellNetravali) { p->b = p->c = 1./3.; }
        d = p;
        break;
    } case EwaLanczosSharp:
    case EwaLanczosSoft: {
        auto p = new EwaLanczosParamSet;
        if (preset == EwaLanczosSharp) {
            p->radius = 3.2383154841662362;
            p->blur = 0.9812505644269356;
        } else if (preset == EwaLanczosSoft) {
            p->radius = 3.2383154841662362;
            p->blur = 1.015;
        }
        d = p;
        break;
    } case Spline36: {
        auto p = new SplineParamSet;
        p->radius = 3;
        d = p;
        break;
    }}
    Q_ASSERT(d);
}

/******************************************************************************/

struct IntrplDialog::Data {
    InterpolatorComboBox *combo = nullptr;
    IntrplParamSetMap map;
    IntrplParamSetWidget *widget = nullptr;
    int page = 1, single = 1;
    bool setting = false;
    auto current() const -> IntrplParamSet { return map[combo->currentEnum()]; }
};

IntrplDialog::IntrplDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    auto vbox = new QVBoxLayout;

    auto hbox = new QHBoxLayout;
    d->combo = new InterpolatorComboBox;
    d->combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(d->combo, &InterpolatorComboBox::currentDataChanged,
            this, [=] () { if (!d->setting) setCurrent(d->map[d->combo->currentEnum()]); });
    hbox->addWidget(d->combo);
    auto reset = new QPushButton(tr("Reset"));
    reset->setDefault(false);
    reset->setAutoDefault(false);
    connect(reset, &QPushButton::clicked, this,
            [=] () { setCurrent(IntrplParamSet(d->combo->currentEnum())); });
    hbox->addWidget(reset);
    vbox->addLayout(hbox);
    d->widget = d->map[Interpolator::Bilinear].createEditor();
    vbox->addWidget(d->widget);
    vbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(vbox);
    adjustSize();
    resize(qMax(width(), 400), height());
}

IntrplDialog::~IntrplDialog()
{
    delete d;
}

auto IntrplDialog::setCurrent(const IntrplParamSet &set) -> void
{
    if (d->widget->type() != set.type()) {
        delete d->widget;
        d->widget = set.createEditor();
        static_cast<QVBoxLayout*>(layout())->insertWidget(1, d->widget);
        d->widget->setParamSet(set);
        connect(d->widget, &IntrplParamSetWidget::changed, this, [=] ()
            { if (!d->setting) emit paramsChanged(d->widget->paramSet()); });
    }
    d->widget->setParamSet(set);
    d->combo->setCurrentEnum(set.type());
    if (!d->setting)
        emit paramsChanged(d->current());
}

auto IntrplDialog::set(Interpolator intrpl, const IntrplParamSetMap &sets) -> void
{
    d->map = sets;
    d->setting = true;
    setCurrent(d->map[intrpl]);
    d->setting = false;
}
