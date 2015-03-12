#include "videofilter.hpp"
#include "tmp/algorithm.hpp"

auto VideoFilter::needsMore() const -> bool
{
    return false;
}

auto PassthroughVideoFilter::push(MpImage &&mpi) -> void
{
    if (mpi.isNull())
        return;
    m_queue.push_back(std::move(mpi));
}

auto PassthroughVideoFilter::pop() -> MpImage
{
    return tmp::take_front(m_queue);
}
