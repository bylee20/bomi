#include "videofilter.hpp"

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
    if (m_queue.empty())
        return MpImage();
    auto mpi = std::move(m_queue.front());
    m_queue.pop_front();
    return mpi;
}
