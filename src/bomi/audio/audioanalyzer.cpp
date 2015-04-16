#include "audioanalyzer.hpp"
#include "misc/log.hpp"
#include "tmp/algorithm.hpp"
#include "kiss_fft/tools/kiss_fftr.h"

// calculate dynamic audio normalization
// basic idea and some codes are taken from DynamicAudioNormalizer
// https://github.com/lordmulder/DynamicAudioNormalizer

class Gaussian {
public:
    Gaussian() { setRadius(1); }
    template<class Iter>
    auto apply(Iter begin, Iter end) const -> double
    {
        Q_ASSERT(std::distance(begin, end) == (int)m_weights.size());
        int i = 0; double ret = 0.0;
        for (auto it = begin; it != end; ++it)
            ret += *it * m_weights[i++];
        return ret;
    }
    auto apply(const std::deque<double> &data) const -> double
    {
        return apply(data.begin(), data.end());
    }
    auto setRadius(int radius) -> void
    {
        const int size = radius * 2 + 1;
        if ((int)m_weights.size() == size)
            return;
        const int shift = m_radius = radius;
        m_weights.resize(size);

        const double sigma = radius / 3.0;
        const double c = 2.0 * pow(sigma, 2);
        auto func = [&] (int i) { return exp(-(pow(i - shift, 2) / c)); };
        double sum = 0.0;
        for(int i = 0; i < size; ++i)
            sum += (m_weights[i] = func(i));
        for(int i = 0; i < size; ++i)
            m_weights[i] /= sum; // normalize
    }
    auto radius() const -> int { return m_radius; }
    auto size() const -> int { return m_weights.size(); }
private:
    int m_radius = 0;
    std::vector<double> m_weights;
};

DECLARE_LOG_CONTEXT(Audio)

struct AudioAnalyzer::FFT::Data {
    kiss_fftr_cfg kiss = nullptr;
    int size_in = 0, pos = 0;
    double nq = 20000;
    std::vector<float> input;
    std::vector<kiss_fft_cpx> output;
    Gaussian gaussian;
};

AudioAnalyzer::FFT::FFT()
    : d(new Data)
{
    setInputSize(10);
//    d->gaussian.setRadius();
}

AudioAnalyzer::FFT::~FFT() { kiss_fftr_free(d->kiss); delete d; }

auto AudioAnalyzer::FFT::inputSize() const -> int
{
    return d->size_in;
}

auto AudioAnalyzer::FFT::setInputSize(int size) -> void
{
    size = kiss_fftr_next_fast_size_real(size);
    if (size != (int)d->input.size()) {
        d->input.resize(size);
        d->size_in = size;
        d->output.resize(d->size_in / 2 + 1);
        kiss_fftr_free(d->kiss);
        d->kiss = kiss_fftr_alloc(d->size_in, false, nullptr, nullptr);
        clear();
    }
}

auto AudioAnalyzer::FFT::push(const AudioBufferPtr &input) -> bool
{
    if (d->pos < d->size_in) {
        if (!input || input->isEmpty())
            return false;
        d->nq = input->fps() / 2;
        auto view = input->constView<float>();
        const float *p = view.plane();
        const int frames = input->frames();
        const int nch = input->channels();
        auto dst = d->input.data() + d->pos;
        for (int i = 0; i < frames && d->pos < d->size_in; ++i, ++d->pos) {
            float mix = 0;
            for (int c = 0; c < nch; ++c) {
                mix += *p++;
            }
            mix /= nch;
            *dst++ = mix;
        }
    }
    return d->pos >= d->size_in;
}

auto AudioAnalyzer::FFT::clear() -> void
{
    d->pos = 0;
}

auto AudioAnalyzer::FFT::pull() -> QList<qreal>
{
    if (d->pos < d->size_in || d->output.size() < 100u)
        return QList<qreal>();
    d->pos = 0;
    kiss_fftr(d->kiss, d->input.data(), d->output.data());

    QList<qreal> ret; ret.reserve(d->output.size());
    for (auto &c : d->output)
        ret.push_back(std::sqrt(c.r * c.r + c.i * c.i));
//    const int size = 100;
//    const int n = d->output.size() / size;
//    QList<qreal> ret; ret.reserve(size);
//    auto p = d->output.data();
//    double max = 0.0;
//    for (int i = 0; i < size; ++i) {

//        double avg = 0.0;
//        for (int j = 0; j < n; ++j, ++p)
//            avg += std::sqrt(p->r * p->r + p->i * p->i);
//        avg /= n;
//        const auto f = d->nq * i / (d->output.size() - 1);
//        if (f > 40)
//            max = std::max(max, avg);
//        ret.push_back(avg);
//    }
//    if (max > 1e-5) {
//        for (auto &v : ret)
//            v /= max;
//        qDebug() << max << d->nq << d->output.size() << (40 * (d->output.size()-1)/d->nq);
//    }
    return ret;
}

/******************************************************************************/

class AudioFrameChunk {
public:
    AudioFrameChunk() { }
    AudioFrameChunk(const AudioBufferFormat &format, const AudioFilter *filter, int frames)
        : m_format(format), m_filter(filter), m_targetFrames(frames) { }
    auto push(AudioBufferPtr buffer) -> AudioBufferPtr
    {
        Q_ASSERT(m_filter);
        if (isFull() || buffer->isEmpty())
            return buffer;
        Q_ASSERT(m_format.channels().num == buffer->channels());
        m_frames += buffer->frames();
        d.push_back(std::move(buffer));
        return AudioBufferPtr();
    }
    auto pop() -> AudioBufferPtr
    {
        auto ret = tmp::take_front(d);
        if (ret)
            m_frames -= ret->frames();
        return ret;
    }
//    auto samples() const -> int { return m_frames * m_format.channels().num; }
    auto frames() const -> int { return m_frames; }
    auto targetFrames() const -> int { return m_targetFrames; }
    auto isFull() const -> bool { return m_frames >= m_targetFrames; }
    auto max(bool *silence) const -> double
    {
        double max = 0.0, avg = 0.0;
        for (auto &buffer : d) {
            auto view = buffer->constView<float>();
            for (float v : view) {
                const auto a = qAbs(v);
                max = std::max<double>(a, max);
                avg += a;
            }
        }
        avg /= m_frames * m_format.channels().num;
        *silence = avg < 1e-4;
        return max;
    }
    auto rms() const -> double
    {
        double sum2 = 0.0;
        for (auto &buffer : d) {
            auto view = buffer->constView<float>();
            for (float v : view)
                sum2 += v * v;
        }
        return sqrt(sum2 / (m_frames * m_format.channels().num));
    }
private:
    AudioBufferFormat m_format;
    std::deque<AudioBufferPtr> d;
    const AudioFilter *m_filter = nullptr;
    int m_frames = 0, m_targetFrames = 0;
};

struct AudioAnalyzer::Data {
    AudioAnalyzer *p = nullptr;
    AudioBufferFormat format;
    AudioNormalizerOption option;
    int frames = 0;
    double scale = 1.0;
    bool normalizer = false;
    struct {
        std::deque<double> orig, min, smooth;
        double prev = 1.0, current = 1.0;
        auto clear() { prev = current = 1.0; orig.clear(); min.clear(); smooth.clear(); }
    } history;
    std::deque<AudioFrameChunk> inputs, outputs;
    AudioFrameChunk filling;
    Gaussian gaussian;

    FFT fft;

    auto chunk() const -> AudioFrameChunk { return { format, p, frames }; }

    auto update(float gain) -> void
    {
        if((int)history.orig.size() < gaussian.radius()) {
            history.current = history.prev = gain;
            history.orig.insert(history.orig.end(), gaussian.radius(), history.prev);
            history.min.insert(history.min.end(), gaussian.radius(), history.prev);
        }
        history.orig.push_back(gain);
#define PUSH_POP(from, to, filter) while((int)from.size() >= gaussian.size()) \
            { to.push_back(filter(from)); from.pop_front(); }
        PUSH_POP(history.orig, history.min, tmp::min);
        PUSH_POP(history.min, history.smooth, gaussian.apply);
#undef PUSH_POP
    }
};

AudioAnalyzer::AudioAnalyzer()
    : d(new Data)
{
    d->p = this;
}

AudioAnalyzer::~AudioAnalyzer()
{
    delete d;
}

auto AudioAnalyzer::setFormat(const AudioBufferFormat &format) -> void
{
    d->history.clear();
    d->fft.clear();
    if (!_Change(d->format, format))
        return;
    d->fft.setInputSize(format.secToFrames(0.1));
    d->format = format;
    reset();
}

auto AudioAnalyzer::reset() -> void
{
    d->inputs.clear();
    d->outputs.clear();
    d->history.smooth.clear();
    d->frames = d->format.secToFrames(d->option.chunk_sec);
    d->filling = d->chunk();
    d->fft.clear();
}

auto AudioAnalyzer::isNormalizerActive() const -> bool
{
    return d->normalizer;
}

auto AudioAnalyzer::setNormalizerActive(bool on) -> void
{
    if (_Change(d->normalizer, on))
        d->history.current = 1.0;
}

auto AudioAnalyzer::gain() const -> float
{
    return d->history.current;
}

auto AudioAnalyzer::setScale(double scale) -> void
{
    d->scale = scale;
}

auto AudioAnalyzer::delay() const -> double
{
    int frames = d->filling.frames();
    for (const auto &chunk : d->inputs)
        frames += chunk.frames();
    for (const auto &chunk : d->outputs)
        frames += chunk.frames();
    return d->format.toSeconds(frames) / d->scale;
}

auto AudioAnalyzer::setNormalizerOption(const AudioNormalizerOption &opt) -> void
{
    d->option.use_rms   = opt.use_rms;
    d->option.smoothing = std::max(1, opt.smoothing);
    d->option.chunk_sec = qBound(0.1, opt.chunk_sec, 1.0);
    d->option.max       = std::min(10.0, opt.max);
    d->option.target    = std::min(0.95, opt.target);

    d->gaussian.setRadius(d->option.smoothing);
    d->history.clear();
    reset();
}

auto AudioAnalyzer::passthrough(const AudioBufferPtr &) const -> bool
{
    return false;
}

auto AudioAnalyzer::push(AudioBufferPtr &src) -> void
{
    Q_ASSERT(!d->filling.isFull());
    AudioBufferPtr left = std::move(src);
    while (left && !left->isEmpty()) {
        left = d->filling.push(left);
        if (d->filling.isFull()) {
            d->inputs.push_back(std::move(d->filling));
            d->filling = d->chunk();
        }
    }
}

auto AudioAnalyzer::fft() const -> FFT*
{
    return &d->fft;
}

static auto cutoff(double value, double cutoff)
{
    constexpr double c = 0.8862269254527580136490837416; //~ sqrt(PI) / 2.0
    return erf(c * (value / cutoff)) * cutoff;
}

auto AudioAnalyzer::pull(bool eof) -> AudioBufferPtr
{
    if (!d->normalizer)
        return flush();
    while (!d->inputs.empty()) {
        auto chunk = tmp::take_front(d->inputs);
        double gain = d->history.prev;
        bool silence = false;
        const auto max = chunk.max(&silence);
        if (!silence) {
            if (d->option.use_rms) {
                const double peak = 0.95 / max;
                const double rms = d->option.target / chunk.rms();
                gain = std::min(peak, rms);
            } else
                gain = d->option.target / max;
        }
        d->update(cutoff(gain, d->option.max));
        d->outputs.push_back(std::move(chunk));
    }
    if (d->history.smooth.empty())
        return eof ? flush() : AudioBufferPtr();
    Q_ASSERT(!d->outputs.empty());
    auto &chunk = d->outputs.front();
    const auto r = qBound(0.0, chunk.frames() / double(chunk.targetFrames()), 1.0);
    d->history.current = d->history.prev * r + (1.0 - r) * d->history.smooth.front();
    auto buffer = d->outputs.front().pop();
    if (!buffer) {
        d->outputs.pop_front();
        d->history.prev = tmp::take_front(d->history.smooth);
    }
    return buffer;
}

auto AudioAnalyzer::flush() -> AudioBufferPtr
{
    auto pop_deque = [&] (auto &d) {
        auto ret = d.front().pop();
        if (!ret) d.pop_front();
        return ret;
    };
    if (!d->outputs.empty())
        return pop_deque(d->outputs);
    if (!d->inputs.empty())
        return pop_deque(d->inputs);
    return d->filling.pop();
}

auto AudioAnalyzer::run(AudioBufferPtr &in) -> AudioBufferPtr
{
    return in;
}
