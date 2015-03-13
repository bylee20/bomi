#include "audionormalizeroption.hpp"
#include "ui_audionormalizeroptionwidget.h"
#include "misc/json.hpp"

#define JSON_CLASS AudioNormalizerOption
static const auto jio = JIO(
    JE(use_rms),
    JE(smoothing),
    JE(max),
    JE(target),
    JE(chunk_sec)
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
    auto signal = &AudioNormalizerOptionWidget::optionChanged;
    PLUG_CHANGED(d->ui.target);
    PLUG_CHANGED(d->ui.use_rms);
    PLUG_CHANGED(d->ui.chunk_sec);
    PLUG_CHANGED(d->ui.max);
    PLUG_CHANGED(d->ui.smoothing);
}

AudioNormalizerOptionWidget::~AudioNormalizerOptionWidget()
{
    delete d;
}

auto AudioNormalizerOptionWidget::option() const -> AudioNormalizerOption
{
    AudioNormalizerOption option;
    option.target = d->ui.target->value();
    option.use_rms = d->ui.use_rms->currentIndex();
    option.chunk_sec = d->ui.chunk_sec->value();
    option.max = d->ui.max->value()/100.0;
    option.smoothing = d->ui.smoothing->value();
    return option;
}

auto AudioNormalizerOptionWidget::setOption(const AudioNormalizerOption &option) -> void
{
    d->ui.target->setValue(option.target);
    d->ui.use_rms->setCurrentIndex(option.use_rms);
    d->ui.chunk_sec->setValue(option.chunk_sec);
    d->ui.max->setValue(option.max * 100.0);
    d->ui.smoothing->setValue(option.smoothing);
}

auto AudioNormalizerOption::default_() -> AudioNormalizerOption
{
    AudioNormalizerOption opt;
    opt.use_rms = false;
    opt.chunk_sec = 0.5;
    opt.smoothing = 15;
    opt.max = 10;
    opt.target = 0.95;
    return opt;
}
