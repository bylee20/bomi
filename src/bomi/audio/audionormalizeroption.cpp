#include "audionormalizeroption.hpp"
#include "ui_audionormalizeroptionwidget.h"
#include "misc/json.hpp"

#define JSON_CLASS AudioNormalizerOption
static const auto jio = JIO(
    JE(silenceLevel),
    JE(minimumGain),
    JE(maximumGain),
    JE(targetLevel),
    JE(bufferLengthInSeconds)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

/******************************************************************************/

struct AudioNormalizerOptionWidget::Data {
    Ui::AudioNormalizerOptionWidget ui;
};

AudioNormalizerOptionWidget::AudioNormalizerOptionWidget(QWidget *parent)
    : QWidget(parent), d(new Data)
{
    d->ui.setupUi(this);
}

AudioNormalizerOptionWidget::~AudioNormalizerOptionWidget()
{
    delete d;
}

auto AudioNormalizerOptionWidget::option() const -> AudioNormalizerOption
{
    AudioNormalizerOption option;
    option.targetLevel = d->ui.target->value();
    option.silenceLevel = d->ui.silence->value();
    option.minimumGain = d->ui.min->value()/100.0;
    option.maximumGain = d->ui.max->value()/100.0;
    option.bufferLengthInSeconds = d->ui.length->value();
    return option;
}

auto AudioNormalizerOptionWidget::setOption(const AudioNormalizerOption &option) -> void
{
    d->ui.silence->setValue(option.silenceLevel);
    d->ui.target->setValue(option.targetLevel);
    d->ui.min->setValue(option.minimumGain * 100.0);
    d->ui.max->setValue(option.maximumGain * 100.0);
    d->ui.length->setValue(option.bufferLengthInSeconds);
}

auto AudioNormalizerOption::default_() -> AudioNormalizerOption
{
    AudioNormalizerOption opt;
    opt.silenceLevel = 0.0001;
    opt.targetLevel = 0.07;
    opt.minimumGain = 0.1;
    opt.maximumGain = 10.0;
    opt.bufferLengthInSeconds = 5.0;
    return opt;
}
