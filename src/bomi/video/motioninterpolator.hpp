#ifndef VIDEODOUBLER_HPP
#define VIDEODOUBLER_HPP

#include "videofilter.hpp"

class MpImage;

class MotionInterpolator : public VideoFilter {
public:
    MotionInterpolator();
    ~MotionInterpolator();
    auto push(MpImage &&mpi) -> void;
    auto pop() -> MpImage;
    auto clear() -> void;
    auto needsMore() const -> bool;
    auto setTargetFps(double fpsManipulation) -> void;
    auto fpsManipulation() const -> double final;
private:
    struct Data;
    Data *d;
};

#endif // VIDEODOUBLER_HPP
