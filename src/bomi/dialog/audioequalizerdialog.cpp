#include "audioequalizerdialog.hpp"
#include "audio/audioequalizer.hpp"

static constexpr int Factor = 10;

using Eq = AudioEqualizer;
using Preset = AudioEqualizer::Preset;

struct AudioEqualizerDialog::Data {
    AudioEqualizerDialog *p = nullptr;
    AudioEqualizer eq;
    std::array<QSlider*, Eq::bands()> sliders;
    Update update;
    QComboBox *presets = nullptr;
};

AudioEqualizerDialog::AudioEqualizerDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    d->p = this;

    auto vbox = new QVBoxLayout;

    auto hbox = new QHBoxLayout;

    hbox->addWidget(new QLabel(tr("Preset")));
    d->presets = new QComboBox;
    d->presets->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    for (int i = 0; i < Eq::MaxPreset; ++i)
        d->presets->addItem(AudioEqualizer::name((Preset)i), i);
    auto sig = static_cast<Signal<QComboBox, int>>(&QComboBox::currentIndexChanged);
    auto load = [=] () {
        auto data = d->presets->currentData();
        if (data.type() == (int)QMetaType::Int)
            setEqualizer(Eq(Preset(data.toInt())));
    };
    connect(d->presets, sig, this, load);
    hbox->addWidget(d->presets);

    auto button = new QPushButton(tr("Load"));
    connect(button, &QPushButton::clicked, this, load);
    hbox->addWidget(button);
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout;
    auto font = this->font();
    font.setPointSizeF(font.pointSizeF()*0.8);
    const int w = QFontMetrics(font).width(u"+20.0dB"_q) + 2;
    for (int i = 0; i < Eq::bands(); ++i) {
        auto s = d->sliders[i] = new QSlider;
        s->setOrientation(Qt::Vertical);
        s->setRange(Eq::min() * Factor, Eq::max() * Factor);
        s->setFixedWidth(w);
        auto vbox = new QVBoxLayout;
        const auto f = Eq::freqeuncy(i);
        const QString fq = f < 999.9 ? (QString::number(f) % "Hz"_a)
                                     : (QString::number(f/1000.0) % "kHz"_a);
        auto label = new QLabel(fq);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);
        vbox->addWidget(label);
        vbox->addWidget(s);
        auto dB = new QLabel("0.0dB"_a);
        dB->setFont(font);
        dB->setAlignment(Qt::AlignCenter);
        vbox->addWidget(dB);
        hbox->addLayout(vbox);

        connect(s, &QSlider::valueChanged, this, [=] () {
            const auto v = s->value()/(double)Factor;
            dB->setText((v > 0 ? "+"_a : ""_a) % QString::number(v, 'f', 1) % "dB"_a);
            if (_Change(d->eq[i], v) && d->update)
                d->update(d->eq);
        });
    }
    vbox->addLayout(hbox);
    setLayout(vbox);

    _SetWindowTitle(this, tr("Audio Equalizer"));
}

AudioEqualizerDialog::~AudioEqualizerDialog()
{
    delete d;
}

auto AudioEqualizerDialog::setEqualizer(const AudioEqualizer &eq) -> void
{
    if (_Change(d->eq, eq)) {
        if (d->update)
            d->update(d->eq);
        for (int i = 0; i < AudioEqualizer::bands(); ++i)
            d->sliders[i]->setValue(qRound(d->eq[i] * Factor));
    }
}

auto AudioEqualizerDialog::equalizer() const -> AudioEqualizer
{
    return d->eq;
}

auto AudioEqualizerDialog::setUpdateFunc(Update &&func) -> void
{
    d->update = std::move(func);
}
