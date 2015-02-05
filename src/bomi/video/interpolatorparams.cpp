#include "interpolatorparams.hpp"
#include "widget/enumcombobox.hpp"
#include "misc/json.hpp"

static const std::array<QByteArray, IntrplParam::TypeMax> s_args = []() {
    std::array<QByteArray, IntrplParam::TypeMax> args;
    args[IntrplParam::Radius] = "radius"_b;
    args[IntrplParam::Param1] = "param1"_b;
    args[IntrplParam::Param2] = "param2"_b;
    args[IntrplParam::AntiRinging] = "antiring"_b;
    return args;
}();

auto IntrplParam::operator == (const IntrplParam &rhs) const -> bool
{
    if (m_type != rhs.m_type || m_available != rhs.m_available)
        return false;
    return !m_available || m_value == rhs.m_value;
}

auto IntrplParam::description() const -> QString
{
    return description(m_type);
}

auto IntrplParam::description(Type type) -> QString
{
    switch (type) {
    case IntrplParam::Radius:
        return tr("Radius");
    case IntrplParam::Param1:
        return tr("Parameter 1");
    case IntrplParam::Param2:
        return tr("Parameter 2");
    case IntrplParam::AntiRinging:
        return tr("Anti-ringing");
    default:
        Q_ASSERT(false);
        return QString();
    }
}

auto IntrplParamSet::default_(Interpolator type) -> IntrplParamSet
{
    IntrplParamSet set;
    auto &params = set.params;
    switch (type) {
    case Interpolator::Bilinear:
        break;
    case Interpolator::Bicubic:
        params[IntrplParam::Param1] = 0.0;
        params[IntrplParam::Param2] = 0.5;
        break;
    case Interpolator::Spline:
        params[IntrplParam::Radius] = { 3, {2, 3, 4} };
        break;
    case Interpolator::Lanczos:
        params[IntrplParam::Radius] = { 3, {2, 3, 4, 5} };
        break;
    case Interpolator::EwaLanczos:
        params[IntrplParam::Radius] = { 3, {2, 3, 4, 5} };
        params[IntrplParam::AntiRinging] = 0.0;
        break;
    case Interpolator::Sharpen:
        params[IntrplParam::Radius] = { 5, {3, 5} };
        params[IntrplParam::Param1] = { 0.5, 0.0, 10 };
        break;
    }
    set.type = type;
    for (uint i = 0; i < set.params.size(); ++i)
        set.params[i].m_type = (IntrplParam::Type)i;
    return set;
}

auto IntrplParamSet::defaults() -> QMap<Interpolator, IntrplParamSet>
{
    IntrplParamSetMap sets;
    for (auto &item : InterpolatorInfo::items())
        sets[item.value] = default_(item.value);
    return sets;
}

auto IntrplParamSet::toMpvOption(const QByteArray &prefix) const -> QByteArray
{
    QByteArray opts;
    opts += _EnumData(type);
    const int radius = params[IntrplParam::Radius].toInt();
    switch (type) {
    case Interpolator::Spline:
        switch (radius) {
        case 2: opts += "16"_b; break;
        case 3: opts += "36"_b; break;
        case 4: opts += "64"_b; break;
        default: Q_ASSERT(false);
        }
        break;
    case Interpolator::Sharpen:
        opts += QByteArray::number(radius);
        break;
    default:
        break;
    }
    for (uint i = 0; i < params.size(); ++i) {
        if (params[i].isAvailable()) {
            opts += ':';
            opts += prefix;
            opts += '-';
            opts += s_args[i];
            opts += '=';
            opts += params[i].toByteArray();
        }
    }
    return opts;
}

auto IntrplParamSet::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert(u"type"_q, _EnumName(type));
    QJsonObject params;
    for (int i = 0; i < IntrplParam::TypeMax; ++i) {
        if (this->params[i].isAvailable())
            params.insert(QString::fromLatin1(s_args[i]), this->params[i].value());
    }
    json.insert(u"params"_q, params);
    return json;
}

auto IntrplParamSet::setFromJson(const QJsonObject &json) -> bool
{
    auto it = json.find(u"type"_q);
    if (it == json.end())
        return false;
    type = _EnumFrom(it.value().toString(), Interpolator::Bilinear);
    this->params = default_(type).params;
    const auto params = json[u"params"_q].toObject();
    for (int i = 0; i < IntrplParam::TypeMax; ++i) {
        it = params.find(QString::fromLatin1(s_args[i]));
        if (it != params.end())
            this->params[i].setValue(it.value().toDouble());
    }
    return true;
}

/******************************************************************************/

struct IntrplDialog::Data {
    InterpolatorComboBox *combo = nullptr;
    struct {
        QSlider *slider = nullptr;
        QLabel *name = nullptr, *value = nullptr;
    } lines[IntrplParam::TypeMax];
    QMap<Interpolator, IntrplParamSet> map;
    bool setting = false;
    auto current() -> IntrplParamSet&
        { auto it = map.find(combo->currentValue()); return it.value(); }
};

IntrplDialog::IntrplDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    auto vbox = new QVBoxLayout;

    auto hbox = new QHBoxLayout;
    d->combo = new InterpolatorComboBox;
    d->combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(d->combo, &InterpolatorComboBox::currentDataChanged,
            this, [=] () { set(d->map[d->combo->currentValue()]); });
    hbox->addWidget(d->combo);
    auto reset = new QPushButton(tr("Reset"));
    connect(reset, &QPushButton::clicked, this,
            [=] () { set(IntrplParamSet::default_(d->combo->currentValue())); });
    hbox->addWidget(reset);
    vbox->addLayout(hbox);

    auto grid = new QGridLayout;
    for (int i = 0; i < IntrplParam::TypeMax; ++i) {
        auto &l = d->lines[i];
        l.slider = new QSlider(Qt::Horizontal);
        l.name = new QLabel(IntrplParam::description((IntrplParam::Type)i));
        l.value = new QLabel;
        l.value->setMinimumWidth(50);

        grid->addWidget(l.name,   i, 0);
        grid->addWidget(l.slider, i, 1);
        grid->addWidget(l.value,  i, 2);

        connect(l.slider, &QSlider::valueChanged, this, [=, &l] (int v) {
            auto &set = d->current();
            auto &param = set.params[i];
            if (param.isInt()) {
                param.setInt(param.validValues()[v]);
                l.value->setText(QString::number(param.toInt()));
            } else {
                param.setValue(v/100.0);
                l.value->setText(QString::number(param.toDouble()));
            }
            emit paramsChanged(set);
        });
    }
    vbox->addLayout(grid);
    setLayout(vbox);
    adjustSize();
    resize(qMax(width(), 400), height());

    set(Interpolator::Bilinear, IntrplParamSet::defaults());
}

IntrplDialog::~IntrplDialog()
{
    delete d;
}

auto IntrplDialog::set(const IntrplParamSet &set) -> void
{
    d->map[set.type] = set;

    d->combo->blockSignals(true);
    d->combo->setCurrentValue(set.type);
    d->combo->blockSignals(false);

    for (int i = 0; i < IntrplParam::TypeMax; ++i) {
        auto &l = d->lines[i];
        auto &param = set.params[i];
        l.slider->blockSignals(true);
        if (param.isAvailable()) {
            if (param.isInt()) {
                l.slider->setRange(0, param.validValues().size() - 1);
                l.slider->setValue(param.validValues().indexOf(param.toInt()));
                l.value->setText(QString::number(param.toInt()));
            } else {
                l.slider->setRange(qRound(param.min() * 100), qRound(param.max() * 100));
                l.slider->setValue(qRound(param.toDouble() * 100));
                l.value->setText(QString::number(param.toDouble()));
            }
        } else
            l.value->setText("--"_a);
        l.name->setEnabled(param.isAvailable());
        l.slider->setEnabled(param.isAvailable());
        l.value->setEnabled(param.isAvailable());
        l.slider->blockSignals(false);
    }

    emit paramsChanged(d->current());
}

auto IntrplDialog::set(Interpolator intrpl, const IntrplParamSetMap &sets) -> void
{
    for (auto &set : sets)
        d->map[set.type] = set;
    blockSignals(true);
    set(d->map[intrpl]);
    blockSignals(false);
}
