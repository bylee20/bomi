#include "visualizer.hpp"
#include "opengl/opengltexture2d.hpp"
#include "player/avinfoobject.hpp"

struct VisualizationHelper::Data {
    AudioObject *audio = nullptr;
    QList<qreal> spectrum;
    qreal min = 40, max = 20000;
    bool active = true;
};

VisualizationHelper::VisualizationHelper(QObject *item)
    : QObject(item), d(new Data)
{

}

VisualizationHelper::~VisualizationHelper()
{
    delete d;
}

auto VisualizationHelper::min() const -> qreal
{
    return d->min;
}

auto VisualizationHelper::max() const -> qreal
{
    return d->max;
}

auto VisualizationHelper::setMin(qreal min) -> void
{
    if (_Change(d->min, min))
        emit minChanged();
}

auto VisualizationHelper::setMax(qreal max) -> void
{
    if (_Change(d->max, max))
        emit maxChanged();
}

auto VisualizationHelper::count() const -> int
{
    return d->spectrum.size();
}

auto VisualizationHelper::setCount(int count) -> void
{
    if (d->spectrum.size() != count) {
        d->spectrum.clear();
        d->spectrum.reserve(count);
        for (int i = 0; i < count; ++i)
            d->spectrum.push_back(0.0);
    }
}

auto VisualizationHelper::audio() const -> AudioObject*
{
    return d->audio;
}

auto VisualizationHelper::setAudio(AudioObject *audio) -> void
{
    if (_Change(d->audio, audio)) {
        connect(d->audio, &AudioObject::spectrumChanged, this, &VisualizationHelper::setSpectrum);
        emit audioChanged();
    }
}

auto VisualizationHelper::spectrum() const -> QList<qreal>
{
    return d->spectrum;
}

auto VisualizationHelper::setSpectrum(const QList<qreal> &orig) -> void
{
    if (!d->active)
        return;
//    function i2f(i, fps, n) { return i * fps * 0.5 / (n - 1); }
    const auto nq = d->audio->filter()->samplerate() * 0.5;
    auto get = [&] (double i) {
        const int left = i;
        const int right = left + 1;
        if (left < 0 || right >= orig.size())
            return 0.0;
        const double a = i - (double)left;
        return orig.at(left) * (1.0 - a) + a * orig.at(right);
    };
    const int count = d->spectrum.size();
    for (int i = 0; i < count; ++i) {
        const auto f = d->min + (d->max - d->min) * i / (count - 1);
        const double idx = f * (orig.size() - 1) / nq;
        double avg = 0.0;
        constexpr int r = 3;
        for (int j = -r; j <= r; ++j)
            avg += get(idx + j);
        d->spectrum[i] = avg / (2 * r + 1);
    }
    emit spectrumChanged();
}

auto VisualizationHelper::isActive() const -> bool
{
    return d->active;
}

auto VisualizationHelper::setActive(bool active) -> void
{
    if (_Change(d->active, active))
        activeChanged();
}
