#include "visualizer.hpp"
#include "opengl/opengltexture2d.hpp"
#include "audiobuffer.hpp"
#include "kiss_fft/tools/kiss_fftr.h"
#include <complex>

static const QEvent::Type UpdateData = QEvent::Type(QEvent::User + 1);

class FFT {
public:
    FFT() { setInputSize(10); }
    ~FFT() { kiss_fftr_free(m_kiss); }
    auto push(const AudioBufferPtr &input) -> bool
    {
        if (m_pos < m_size_in) {
            if (!input || input->isEmpty())
                return false;
            m_nq = input->fps() / 2;
            auto view = input->constView<float>();
            const float *p = view.plane();
            const int frames = input->frames();
            const int nch = input->channels();
            auto dst = m_input.data() + m_pos;
            for (int i = 0; i < frames && m_pos < m_size_in; ++i, ++m_pos) {
                float mix = 0;
                for (int c = 0; c < nch; ++c) {
                    mix += *p++;
                }
                mix /= nch;
                *dst++ = mix;
            }
        }
        return m_pos >= m_size_in;
    }
    auto run() -> void { kiss_fftr(m_kiss, m_input.data(), (kiss_fft_cpx*)m_output.data()); clear(); }
    auto output() -> const std::vector<std::complex<float>>& { return m_output; }
    auto inputSize() const -> int { return m_size_in; }
    auto setInputSize(int size) -> void
    {
        static_assert(sizeof(std::complex<float>) == sizeof(kiss_fft_cpx), "!!!");
        size = kiss_fftr_next_fast_size_real(size);
        if (size != (int)m_input.size()) {
            m_input.resize(size);
            m_size_in = size;
            m_output.resize(m_size_in / 2 + 1);
            kiss_fftr_free(m_kiss);
            m_kiss = kiss_fftr_alloc(m_size_in, false, nullptr, nullptr);
        }
        clear();
    }
    auto clear() -> void { m_pos = 0; }
private:
    kiss_fftr_cfg m_kiss = nullptr;
    int m_size_in = 0, m_pos = 0;
    double m_nq = 20000;
    std::vector<float> m_input;
    std::vector<std::complex<float>> m_output;
};

/******************************************************************************/

struct AudioVisualizer::Data {
    QList<qreal> data, interm, back;
    qreal min = 20, max = 20000;
    bool active = false, enabled = false;
    int fps = 0, count = 0;
    double minLv = _Max<double>(), maxLv = 0;
    Type type = None;
    QMutex mutex;
    FFT fft;
    AudioVisualizer::Scale xs = AudioVisualizer::Log;
    AudioVisualizer::Scale ys = AudioVisualizer::Log, tys = ys;
};

AudioVisualizer::AudioVisualizer(QObject *item)
    : QObject(item), d(new Data)
{
    setCount(5);
}

AudioVisualizer::~AudioVisualizer()
{
    delete d;
}

auto AudioVisualizer::reset() -> void
{
    d->maxLv = 0.0;
    d->minLv = _Max<double>();
}

//        function i2f(i, fps, n) { return i * fps * 0.5 / (n - 1); }
auto AudioVisualizer::analyze(const QSharedPointer<AudioBuffer> &data) -> void
{
    if (!d->enabled)
        return;
    Q_ASSERT(data);
    if (_Change(d->fps, data->fps()))
        d->fft.setInputSize(d->fps * 0.1);
    if (!d->fft.push(data))
        return;
    d->fft.run();
    auto &cpx = d->fft.output();

    const int c = d->count;
    if (d->back.size() != c) {
        d->back.clear(); d->back.reserve(c);
        for (int i = 0; i < c; ++i)
            d->back.push_back(0.0);
    }
    const auto nq = d->fps * 0.5;
    auto get = [&] (double i) -> double {
        const int left = i;
        const int right = left + 1;
        if (left < 0 || right >= (int)cpx.size())
            return 0.0;
        const float a = i - (double)left;
        return std::abs(cpx.at(left)) * (1.0f - a) + a * std::abs(cpx.at(right));
    };

    constexpr int radius = 3;
    static const auto gw = Gaussian::create(radius);

    if (_Change(d->tys, d->ys))
        reset();

    double &min = d->minLv, &max = d->maxLv;
    for (int i = 0; i < c; ++i) {
        const auto f = d->xs != Log ? d->min + (d->max - d->min) * i / (c - 1)
            : std::exp(std::log(d->min) + (std::log(d->max) - std::log(d->min)) * i / (c - 1));
        const double idx = f * (cpx.size() - 1) / nq;
        double lv = 0.0;
        int g = 0;
        for (int j = -radius; j <= radius; ++j, ++g)
            lv += get(idx + j) * gw[g];
        if (lv < 1e-4)
            lv = 0.0;
        else {
            if (d->tys == Log)
                lv = std::log(lv);
            min = std::min(lv, min);
            max = std::max(lv, max);
        }
        d->back[i] = lv;
    }
    if (d->tys != Log)
        min = 0;
    if (min != max) {
        for (auto &v : d->back) {
            if (v != 0.0)
                v = (v - min) / (max - min);
        }
    }

    d->mutex.lock();
    d->back.swap(d->interm);
    d->mutex.unlock();
    qApp->postEvent(this, new QEvent(UpdateData));
}

auto AudioVisualizer::min() const -> qreal
{
    return d->min;
}

auto AudioVisualizer::max() const -> qreal
{
    return d->max;
}

auto AudioVisualizer::setMin(qreal min) -> void
{
    if (_Change(d->min, min))
        emit minChanged();
}

auto AudioVisualizer::setMax(qreal max) -> void
{
    if (_Change(d->max, max))
        emit maxChanged();
}

auto AudioVisualizer::count() const -> int
{
    return d->count;
}

auto AudioVisualizer::setCount(int count) -> void
{
    if (_Change(d->count, count))
        emit countChanged();
}

auto AudioVisualizer::setEnabled(bool enabled) -> void
{
    if (_Change(d->enabled, enabled))
        emit enabledChanged();
}

auto AudioVisualizer::isEnabled() const -> bool
{
    return d->enabled;
}

auto AudioVisualizer::data() const -> QList<qreal>
{
    return d->data;
}

auto AudioVisualizer::isActive() const -> bool
{
    return d->active;
}

auto AudioVisualizer::setActive(bool active) -> void
{
    if (_Change(d->active, active)) {
        emit activeChanged();
        setEnabled(d->active && d->type);
    }
}

auto AudioVisualizer::customEvent(QEvent *e) -> void
{
    if (e->type() == UpdateData) {
        d->mutex.lock();
        d->data.swap(d->interm);
        d->mutex.unlock();
        emit dataChanged();
    }
}

auto AudioVisualizer::setXScale(Scale scale) -> void
{
    if (_Change(d->xs, scale))
        emit xScaleChanged();
}

auto AudioVisualizer::xScale() const -> Scale
{
    return d->xs;
}

auto AudioVisualizer::setYScale(Scale scale) -> void
{
    if (_Change(d->ys, scale))
        emit yScaleChanged();
}

auto AudioVisualizer::yScale() const -> Scale
{
    return d->ys;
}

auto AudioVisualizer::type() const -> Type
{
    return d->type;
}

auto AudioVisualizer::setType(Visualization type) -> void
{
    if (_Change(d->type, (Type)type)) {
        emit typeChanged();
        setEnabled(d->active && d->type);
    }
}
