#ifndef VIDEOFILTER_HPP
#define VIDEOFILTER_HPP

#include "mpimage.hpp"

class VideoFilter {
public:
    virtual auto push(MpImage &&mpi) -> void = 0;
    virtual auto pop() -> MpImage = 0;
    virtual auto clear() -> void = 0;
    virtual auto needsMore() const -> bool;
    virtual auto fpsManipulation() const -> double { return 1.0; }
};

class PassthroughVideoFilter : public VideoFilter {
public:
    auto push(MpImage &&mpi) -> void override;
    auto pop() -> MpImage override;
    auto clear() -> void override { m_queue.clear(); }
private:
    std::deque<MpImage> m_queue;
};

#endif // VIDEOFILTER_HPP
