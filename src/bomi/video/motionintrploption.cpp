#include "motionintrploption.hpp"
#include "os/os.hpp"
#include "misc/json.hpp"
#include <QRadioButton>

#define JSON_CLASS MotionIntrplOption

static const auto jio = JIO(JE(sync_to_monitor), JE(target_fps));

JSON_DECLARE_FROM_TO_FUNCTIONS

auto MotionIntrplOption::fps() const -> double
{
    if (sync_to_monitor)
        return OS::refreshRate();
    return target_fps;
}

enum Button { Sync, Target };

struct MotionIntrplOptionWidget::Data {
    QButtonGroup *g = nullptr;
    QLabel *detected = nullptr;
    QDoubleSpinBox *fps = nullptr;
};

MotionIntrplOptionWidget::MotionIntrplOptionWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    d->g = new QButtonGroup(this);
    d->g->setExclusive(true);

    auto vbox = new QVBoxLayout;

    auto r1 = new QRadioButton(tr("Sync to refresh rate of monitor"));
    d->detected = new QLabel;
    auto hbox = new QHBoxLayout;
    hbox->addWidget(r1);
    hbox->addWidget(d->detected);
    hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    vbox->addLayout(hbox);

    auto r2 = new QRadioButton(tr("Target framerate"));
    d->fps = new QDoubleSpinBox;
    d->fps->setSuffix(" fps"_a);
    d->fps->setDecimals(2);
    d->fps->setMaximum(999);
    d->fps->setMinimum(10);
    hbox = new QHBoxLayout;
    hbox->addWidget(r2);
    hbox->addWidget(d->fps);
    hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    vbox->addLayout(hbox);

    setLayout(vbox);

    d->g->addButton(r1, Sync);
    d->g->addButton(r2, Target);

    auto signal = &MotionIntrplOptionWidget::optionChanged;
    PLUG_CHANGED(d->g);
    PLUG_CHANGED(d->fps);
    connect(r2, &QRadioButton::toggled, d->fps, &QWidget::setEnabled);
    d->fps->setEnabled(false);
}

MotionIntrplOptionWidget::~MotionIntrplOptionWidget()
{
    delete d;
}

auto MotionIntrplOptionWidget::option() const -> MotionIntrplOption
{
    MotionIntrplOption option;
    option.sync_to_monitor = d->g->checkedId() == Sync;
    option.target_fps = d->fps->value();
    return option;
}

auto MotionIntrplOptionWidget::setOption(const MotionIntrplOption &option) -> void
{
    if (d->g->button(Sync)->isEnabled())
        d->g->button(option.sync_to_monitor ? Sync : Target)->setChecked(true);
    else
        d->g->button(Target)->setChecked(true);
    d->fps->setValue(option.target_fps);
}

auto MotionIntrplOptionWidget::showEvent(QShowEvent *e) -> void
{
    QGroupBox::showEvent(e);
    const QString text = '('_q % tr("detected: %1Hz") % ')'_q;
    auto rf = OS::refreshRate();
    d->g->button(Sync)->setEnabled(rf > 0);
    d->g->button(Target)->setChecked(rf < 0);
    d->detected->setText(text.arg(rf < 0 ? u"--"_q : QString::number(rf, 'f', 2)));
}
