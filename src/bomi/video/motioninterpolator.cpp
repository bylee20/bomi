#include "motioninterpolator.hpp"
#include "mpimage.hpp"
#include "misc/log.hpp"
#include "tmp/algorithm.hpp"

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

    auto pts2us(double pts) -> int64_t
    {
        if (pts == MP_NOPTS_VALUE)
            return 0;
        return pts * 1e6;
    }

    auto push(MpImage &&mpi, double pts, int additional) -> void
    {
//        auto &timing = mpi->frame_timing;
//        timing.pts = pts2us(mpi->pts);
//        timing.next_vsync = pts2us(pts);
//        timing.prev_vsync = timing.next_vsync - pts2us(dt > 0 ? dt : 0);

        mpi->pts = pts;
        mpi->fields |= additional;
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
        int additional = 0;
        do {
            d->push(std::move(MpImage(mpi)), d->next(), additional);
            additional = MP_IMGFIELD_ADDITIONAL;
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
    return tmp::take_front(d->queue);
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
