#include "motioninterpolator.hpp"
#include "mpimage.hpp"

struct MotionInterpolator::Data {
    MotionInterpolator *p = nullptr;
    std::deque<MpImage> queue;
    bool eof = false;
    double dt = -1;
    auto next() const -> double
    {
        Q_ASSERT(!queue.empty() && dt > 0);
        return queue.back()->pts + dt;
    }

    auto push(MpImage &&mpi, double pts, int additional) -> void
    {
        mpi->pts_orig = mpi->pts;
        mpi->pts = pts;
        if (additional)
            mpi->fields |= MP_IMGFIELD_ADDITIONAL;
        queue.push_back(std::move(mpi));
    }
};

MotionInterpolator::MotionInterpolator()
    : d(new Data)
{
    d->p = this;
}

MotionInterpolator::~MotionInterpolator()
{
    delete d;
}

auto MotionInterpolator::push(MpImage &&mpi) -> void
{
    d->eof = mpi.isNull();
    if (d->eof)
        return;
    if (d->queue.empty() || d->dt < 0 || mpi->pts < d->next())
        d->push(std::move(mpi), mpi->pts, false);
    else {
        bool additional = false;
        do {
            d->push(std::move(MpImage(mpi)), d->next(), additional);
            additional = true;
        } while (d->next() < mpi->pts);
    }
}

auto MotionInterpolator::needsMore() const -> bool
{
    return !d->eof && d->queue.size() < 2;
}

auto MotionInterpolator::pop() -> MpImage
{
    if (d->queue.empty() || (!d->eof && d->queue.size() < 2))
        return MpImage();
    auto ret = std::move(d->queue.front());
    d->queue.pop_front();
    return std::move(ret);
}

auto MotionInterpolator::clear() -> void
{
    d->queue.clear();
    d->eof = false;
}

auto MotionInterpolator::setTargetFps(double fps) -> void
{
    d->dt = 1.0/fps;
}

auto MotionInterpolator::fpsManipulation() const -> double
{
    return 1.0/d->dt;
}
