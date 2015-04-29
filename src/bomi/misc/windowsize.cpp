#include "windowsize.hpp"
#include <QAction>

auto WindowSize::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert(u"display_based"_q, display_based);
    json.insert(u"rate"_q, rate);
    return json;
}

auto WindowSize::setFromJson(const QJsonObject &json) -> bool
{
    WindowSize ws;
    auto it = json.find(u"display_based"_q);
    if (it == json.end())
        return false;
    ws.display_based = it.value().toBool(false);
    it = json.find(u"rate"_q);
    if (it == json.end())
        return false;
    ws.rate = it.value().toDouble(1.0);
    *this = ws;
    return true;
}

auto WindowSize::baseText(bool dis) -> QString
{
    if (dis)
        return qApp->translate("WindowSize", "Display Size");
    return qApp->translate("WindowSize", "Video Size");
}

auto WindowSize::fillAction(QAction *action) const -> void
{
    action->setText(baseText(display_based) % " x"_a % _N(qRound(rate * 100)) %
                    qApp->translate("PrefDialog", " %"));
    action->setData(QVariant::fromValue(*this));
}


auto WindowSize::defaults() -> QList<WindowSize>
{
    static const QList<WindowSize> list = {
        { true,  0.1 },
        { true,  0.2 },
        { true,  0.3 },
        { true,  0.4 },
        { false, 1.0 }
    };
    return list;
}

/******************************************************************************/

static constexpr int s_count = 5;

struct WindowSizeWidget::Data {
    struct {
        QComboBox *base;
        QSpinBox *percent;
    } widgets[s_count];
};

WindowSizeWidget::WindowSizeWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    auto grid = new QGridLayout(this);

    auto signal = &WindowSizeWidget::valuesChanged;
    for (int i = 0; i < s_count; ++i) {
        auto base = d->widgets[i].base = new QComboBox;
        auto pcnt = d->widgets[i].percent = new QSpinBox;

        int c = 0;
        grid->addWidget(new QLabel(tr("Action %1").arg(i + 1) % ':'_q), i, c++);
        grid->addWidget(base, i, c++);
        grid->addWidget(new QLabel(u"x"_q), i, c++);
        grid->addWidget(pcnt, i, c++);
        grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), i, c++);

        for (int i = 0; i < 2; ++i)
            base->addItem(WindowSize::baseText(i));
        pcnt->setRange(10, 900);
        pcnt->setSingleStep(5);
        pcnt->setAccelerated(true);
        pcnt->setSuffix(qApp->translate("PrefDialog", " %"));

        PLUG_CHANGED(base);
        PLUG_CHANGED(pcnt);
    }
}

WindowSizeWidget::~WindowSizeWidget()
{
    delete d;
}

auto WindowSizeWidget::values() const -> QList<WindowSize>
{
    QList<WindowSize> values; values.reserve(s_count);
    for (int i = 0; i < s_count; ++i) {
        auto &w = d->widgets[i];
        values.push_back({ w.base->currentIndex(), w.percent->value() * 1e-2 });
    }
    Q_ASSERT(values.size() == s_count);
    return values;
}

auto WindowSizeWidget::setValues(const QList<WindowSize> &values) -> void
{
    Q_ASSERT(values.size() == s_count);
    for (int i = 0; i < s_count; ++i) {
        auto &w = d->widgets[i];
        w.base->setCurrentIndex(values[i].display_based);
        w.percent->setValue(qRound(values[i].rate * 100));
    }
}
